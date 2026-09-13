#include "RolandSysex.h"

#include <libremidi/libremidi.hpp>

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <condition_variable>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <mutex>
#include <optional>
#include <queue>
#include <span>
#include <sstream>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

namespace {

constexpr int kPatchCount = 128;
constexpr auto kInterRequestDelay = std::chrono::milliseconds(50);

struct Options {
  bool listOnly = false;
  // Default full-dump path: listen for panel bulk dump (proven on GP-16).
  // Use --request for host-initiated RQ1 per patch.
  bool requestMode = false;
  bool verbose = false;
  std::uint8_t deviceId = 0x00;
  std::string inPort;
  std::string outPort;
  std::string outFile = "gp16-full-dump.bin";
  std::string api = "alsa_raw";
  int timeoutMs = 8000;
  int retries = 2;
  int patchFrom = 0;
  int patchTo = 127;
  int listenSeconds = 180;
};

libremidi::API resolveApi(const std::string& name)
{
  if (name == "default" || name.empty())
    return libremidi::midi1::default_api();
  if (name == "alsa_raw" || name == "raw")
    return libremidi::API::ALSA_RAW;
  if (name == "alsa_seq" || name == "seq")
    return libremidi::API::ALSA_SEQ;
  throw std::runtime_error("Unknown --api value: " + name + " (use alsa_raw|alsa_seq|default)");
}

void usage(const char* argv0)
{
  std::cerr
      << "Usage: " << argv0 << " [options]\n"
      << "\n"
      << "Full GP-16 patch dump over MIDI SysEx.\n"
      << "\n"
      << "Default mode is --listen (panel bulk dump). This matches how the GP-16\n"
      << "normally transmits all 128 patches. Use --request for host RQ1 pulls.\n"
      << "\n"
      << "Options:\n"
      << "  -l, --list              List MIDI ports and exit\n"
      << "  -i, --in <name|#>      MIDI input port (name substring or index)\n"
      << "  -o, --out <name|#>     MIDI output port (required for --request)\n"
      << "  -d, --device-id <hex>  Roland device/unit ID (default: 00)\n"
      << "  -f, --file <path>      Output file (default: gp16-full-dump.bin)\n"
      << "      --listen           Listen for panel bulk dump (default)\n"
      << "      --request          Host RQ1 for Group A then Group B (01 00 00 / 01 40 00)\n"
      << "      --listen-seconds N Listen timeout (default: 180)\n"
      << "  -t, --timeout <ms>     Per-group RQ1 timeout (default: 8000)\n"
      << "  -r, --retries <n>      RQ1 retries per patch (default: 2)\n"
      << "      --from <0-127>     First patch index (default: 0)\n"
      << "      --to <0-127>       Last patch index (default: 127)\n"
      << "      --api <name>       alsa_raw (default), alsa_seq, default\n"
      << "  -v, --verbose          Log each message\n"
      << "  -h, --help             Show this help\n"
      << "\n"
      << "Examples:\n"
      << "  " << argv0 << " --list\n"
      << "  " << argv0 << " -i \"USB MIDI\" -f dump.bin\n"
      << "      # then start bulk dump on the GP-16 panel\n"
      << "  " << argv0 << " --request -i \"USB MIDI\" -o \"USB MIDI\" -d 00 -f dump.bin\n";
}

std::string portName(const libremidi::port_information& p)
{
  if (!p.display_name.empty())
    return p.display_name;
  if (!p.port_name.empty())
    return p.port_name;
  if (!p.device_name.empty())
    return p.device_name;
  return "(unnamed)";
}

std::optional<std::size_t> resolvePortIndex(
    const std::vector<std::string>& names, const std::string& key)
{
  if (key.empty())
    return std::nullopt;

  bool allDigit = !key.empty();
  for (char c : key) {
    if (c < '0' || c > '9') {
      allDigit = false;
      break;
    }
  }
  if (allDigit) {
    const auto idx = static_cast<std::size_t>(std::stoul(key));
    if (idx < names.size())
      return idx;
    return std::nullopt;
  }

  for (std::size_t i = 0; i < names.size(); ++i) {
    if (names[i] == key)
      return i;
  }
  for (std::size_t i = 0; i < names.size(); ++i) {
    if (names[i].find(key) != std::string::npos)
      return i;
  }
  auto lower = [](std::string s) {
    for (char& c : s)
      c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return s;
  };
  const auto keyL = lower(key);
  for (std::size_t i = 0; i < names.size(); ++i) {
    if (lower(names[i]).find(keyL) != std::string::npos)
      return i;
  }
  return std::nullopt;
}

std::uint8_t parseHexByte(std::string_view s)
{
  if (s.size() >= 2 && s[0] == '0' && (s[1] == 'x' || s[1] == 'X'))
    s.remove_prefix(2);
  return static_cast<std::uint8_t>(std::stoul(std::string(s), nullptr, 16) & 0x7F);
}

bool parseArgs(int argc, char** argv, Options& opt)
{
  for (int i = 1; i < argc; ++i) {
    const std::string a = argv[i];
    auto need = [&](const char* name) -> std::string {
      if (i + 1 >= argc)
        throw std::runtime_error(std::string("missing value for ") + name);
      return argv[++i];
    };

    if (a == "-h" || a == "--help") {
      usage(argv[0]);
      std::exit(0);
    } else if (a == "-l" || a == "--list") {
      opt.listOnly = true;
    } else if (a == "--listen") {
      opt.requestMode = false;
    } else if (a == "--request") {
      opt.requestMode = true;
    } else if (a == "-v" || a == "--verbose") {
      opt.verbose = true;
    } else if (a == "--api") {
      opt.api = need(a.c_str());
    } else if (a == "--listen-seconds") {
      opt.listenSeconds = std::stoi(need(a.c_str()));
    } else if (a == "-i" || a == "--in") {
      opt.inPort = need(a.c_str());
    } else if (a == "-o" || a == "--out") {
      opt.outPort = need(a.c_str());
    } else if (a == "-d" || a == "--device-id") {
      opt.deviceId = parseHexByte(need(a.c_str()));
    } else if (a == "-f" || a == "--file") {
      opt.outFile = need(a.c_str());
    } else if (a == "-t" || a == "--timeout") {
      opt.timeoutMs = std::stoi(need(a.c_str()));
    } else if (a == "-r" || a == "--retries") {
      opt.retries = std::stoi(need(a.c_str()));
    } else if (a == "--from") {
      opt.patchFrom = std::stoi(need(a.c_str()));
    } else if (a == "--to") {
      opt.patchTo = std::stoi(need(a.c_str()));
    } else {
      std::cerr << "Unknown option: " << a << "\n";
      usage(argv[0]);
      return false;
    }
  }

  if (opt.patchFrom < 0 || opt.patchTo > 127 || opt.patchFrom > opt.patchTo) {
    std::cerr << "Invalid --from/--to range\n";
    return false;
  }
  return true;
}

std::string toHex(std::span<const std::uint8_t> data, std::size_t maxBytes = 32)
{
  std::ostringstream oss;
  const auto n = std::min(maxBytes, data.size());
  for (std::size_t i = 0; i < n; ++i) {
    if (i)
      oss << ' ';
    oss.width(2);
    oss.fill('0');
    oss << std::hex << std::uppercase << static_cast<int>(data[i]);
  }
  if (data.size() > maxBytes)
    oss << " …";
  return oss.str();
}

std::string errorText(const stdx::error& err)
{
  const auto ref = err.message();
  return std::string(ref.data(), ref.size());
}

std::string extractName(const roland::ParsedDt1& dt1)
{
  return roland::extractPatchName(dt1.data);
}

class SysexInbox {
public:
  void push(std::vector<std::uint8_t> msg)
  {
    {
      std::scoped_lock lock(mutex_);
      queue_.push(std::move(msg));
    }
    cv_.notify_one();
  }

  std::vector<std::vector<std::uint8_t>> drainFor(std::chrono::milliseconds window)
  {
    std::vector<std::vector<std::uint8_t>> out;
    const auto deadline = std::chrono::steady_clock::now() + window;
    std::unique_lock lock(mutex_);
    while (std::chrono::steady_clock::now() < deadline) {
      while (!queue_.empty()) {
        out.push_back(std::move(queue_.front()));
        queue_.pop();
      }
      cv_.wait_until(lock, deadline);
    }
    while (!queue_.empty()) {
      out.push_back(std::move(queue_.front()));
      queue_.pop();
    }
    return out;
  }

private:
  mutable std::mutex mutex_;
  std::condition_variable cv_;
  std::queue<std::vector<std::uint8_t>> queue_;
};

int collectGroupPayload(
    SysexInbox& inbox,
    std::vector<std::vector<std::uint8_t>>& stored,
    std::vector<std::uint8_t>& payload,
    int expected,
    std::chrono::milliseconds timeout,
    bool verbose)
{
  payload.clear();
  stored.clear();
  const auto deadline = std::chrono::steady_clock::now() + timeout;
  while (static_cast<int>(payload.size()) < expected
         && std::chrono::steady_clock::now() < deadline) {
    auto msgs = inbox.drainFor(std::chrono::milliseconds(200));
    for (auto& msg : msgs) {
      auto parsed = roland::parseDt1(msg);
      if (!parsed.valid)
        continue;
      payload.insert(payload.end(), parsed.data.begin(), parsed.data.end());
      stored.push_back(std::move(msg));
      if (verbose) {
        std::cout << "  RX DT1 " << stored.back().size() << " B, payload "
                  << payload.size() << "/" << expected << "\n";
      } else {
        std::cout << "\r  payload " << payload.size() << "/" << expected << std::flush;
      }
    }
  }
  if (!verbose)
    std::cout << "\n";
  return static_cast<int>(payload.size());
}

} // namespace

int main(int argc, char** argv)
{
  Options opt;
  try {
    if (!parseArgs(argc, argv, opt))
      return 2;
  } catch (const std::exception& ex) {
    std::cerr << "Argument error: " << ex.what() << "\n";
    return 2;
  }

  const auto api = resolveApi(opt.api);
  std::cout << "API: " << libremidi::get_api_display_name(api) << "\n";
  std::cout << "Mode: " << (opt.requestMode ? "request (RQ1)" : "listen (panel dump)") << "\n";

  libremidi::observer observer{
      {
          .track_hardware = true,
          .track_virtual = true,
      },
      libremidi::observer_configuration_for(api),
  };
  std::this_thread::sleep_for(std::chrono::milliseconds(150));

  auto inputs = observer.get_input_ports();
  auto outputs = observer.get_output_ports();

  std::vector<std::string> inNames;
  std::vector<std::string> outNames;
  for (const auto& p : inputs)
    inNames.push_back(portName(p));
  for (const auto& p : outputs)
    outNames.push_back(portName(p));

  if (opt.listOnly || opt.inPort.empty()) {
    std::cout << "MIDI inputs (" << inNames.size() << "):\n";
    for (std::size_t i = 0; i < inNames.size(); ++i)
      std::cout << "  [" << i << "] " << inNames[i] << "\n";
    std::cout << "MIDI outputs (" << outNames.size() << "):\n";
    for (std::size_t i = 0; i < outNames.size(); ++i)
      std::cout << "  [" << i << "] " << outNames[i] << "\n";
    if (opt.listOnly)
      return 0;
    if (opt.inPort.empty()) {
      std::cerr << "\nSpecify -i input port (or --list).\n";
      return 2;
    }
  }

  auto prefer = [](const std::vector<std::string>& names, const std::string& hint) {
    auto idx = resolvePortIndex(names, hint);
    if (idx)
      return idx;
    return resolvePortIndex(names, "USB MIDI");
  };

  if (opt.inPort.empty())
    opt.inPort = "USB MIDI";
  if (opt.requestMode && opt.outPort.empty())
    opt.outPort = "USB MIDI";

  const auto inIdx = prefer(inNames, opt.inPort);
  if (!inIdx) {
    std::cerr << "Input port not found: " << opt.inPort << "\n";
    return 1;
  }

  std::optional<std::size_t> outIdx;
  if (opt.requestMode) {
    outIdx = prefer(outNames, opt.outPort);
    if (!outIdx) {
      std::cerr << "Output port not found: " << opt.outPort << "\n";
      return 1;
    }
  }

  std::cout << "Input:  [" << *inIdx << "] " << inNames[*inIdx] << "\n";
  if (outIdx)
    std::cout << "Output: [" << *outIdx << "] " << outNames[*outIdx] << "\n";
  std::cout << "Device ID: 0x" << std::hex << static_cast<int>(opt.deviceId) << std::dec << "\n";
  std::cout << "File: " << opt.outFile << "\n";

  SysexInbox inbox;
  std::vector<std::uint8_t> partial;
  bool inSysex = false;

  libremidi::midi_in midiin{
      {
          .on_message =
              [&](libremidi::message&& message) {
                std::vector<std::uint8_t> bytes(message.begin(), message.end());
                if (bytes.empty())
                  return;
                if (bytes.front() == 0xF0 && bytes.back() == 0xF7) {
                  inbox.push(std::move(bytes));
                  return;
                }
                for (auto b : bytes) {
                  if (b == 0xF0) {
                    inSysex = true;
                    partial.clear();
                    partial.push_back(b);
                  } else if (inSysex) {
                    partial.push_back(b);
                    if (b == 0xF7) {
                      inSysex = false;
                      inbox.push(partial);
                      partial.clear();
                    }
                  }
                }
              },
          .on_error =
              [](std::string_view text, const libremidi::source_location&) {
                std::cerr << "MIDI error: " << text << "\n";
              },
          .ignore_sysex = false,
          .ignore_timing = true,
          .ignore_sensing = true,
      },
      libremidi::midi_in_configuration_for(api),
  };

  if (auto err = midiin.open_port(inputs[*inIdx]); err != stdx::error{}) {
    std::cerr << "Failed to open input: " << errorText(err) << "\n";
    return 1;
  }

  std::unique_ptr<libremidi::midi_out> midiout;
  if (opt.requestMode) {
    midiout = std::make_unique<libremidi::midi_out>(
        libremidi::output_configuration{},
        libremidi::midi_out_configuration_for(api));
    if (auto err = midiout->open_port(outputs[*outIdx]); err != stdx::error{}) {
      std::cerr << "Failed to open output: " << errorText(err) << "\n";
      return 1;
    }
  }

  std::vector<std::vector<std::uint8_t>> patches(
      static_cast<std::size_t>(opt.patchTo - opt.patchFrom + 1));
  int received = 0;
  const auto timeout = std::chrono::milliseconds(opt.timeoutMs);

  if (!opt.requestMode) {
    std::cout << "Listening for panel bulk dump (up to " << opt.listenSeconds
              << "s). Start MIDI bulk dump / transmit on the GP-16 now.\n";

    const auto overall
        = std::chrono::steady_clock::now() + std::chrono::seconds(opt.listenSeconds);
    std::vector<bool> got(kPatchCount, false);
    int count = 0;
    auto lastRx = std::chrono::steady_clock::now();

    while (std::chrono::steady_clock::now() < overall
           && count < (opt.patchTo - opt.patchFrom + 1)) {
      auto msgs = inbox.drainFor(std::chrono::milliseconds(200));
      for (auto& msg : msgs) {
        auto parsed = roland::parseDt1(msg);
        if (!parsed.valid || parsed.address.size() < 2 || parsed.address[0] != 0x0F)
          continue;
        const int idx = parsed.address[1];
        if (idx < opt.patchFrom || idx > opt.patchTo)
          continue;
        const auto slot = static_cast<std::size_t>(idx - opt.patchFrom);
        if (got[static_cast<std::size_t>(idx)])
          continue;
        got[static_cast<std::size_t>(idx)] = true;
        patches[slot] = std::move(msg);
        ++received;
        ++count;
        lastRx = std::chrono::steady_clock::now();
        if (opt.verbose) {
          std::cout << "  patch " << idx << " \"" << extractName(parsed) << "\" ("
                    << patches[slot].size() << " B)\n";
        } else {
          std::cout << "\rReceived " << received << "/"
                    << (opt.patchTo - opt.patchFrom + 1) << " patches" << std::flush;
        }
      }
      // Stop early if we got a full set and traffic quieted
      if (received >= (opt.patchTo - opt.patchFrom + 1)
          && std::chrono::steady_clock::now() - lastRx > std::chrono::seconds(1))
        break;
    }
    if (!opt.verbose)
      std::cout << "\n";
  } else {
    std::cout << "Requesting Group A (01 00 00) then Group B (01 40 00), size 00 40 00…\n";

    const bool needA = opt.patchFrom <= 63;
    const bool needB = opt.patchTo >= 64;
    std::vector<std::vector<std::uint8_t>> requestMsgs;
    std::vector<std::uint8_t> payloadA;
    std::vector<std::uint8_t> payloadB;

    auto requestGroup = [&](std::span<const std::uint8_t, 3> address, const char* label,
                            std::vector<std::uint8_t>& payload) -> bool {
      const auto rq1 = roland::buildRequestData(opt.deviceId, address, roland::kInternalGroupSize);
      for (int attempt = 0; attempt <= opt.retries; ++attempt) {
        inbox.drainFor(std::chrono::milliseconds(0));
        if (auto err = midiout->send_message(rq1.data(), rq1.size()); err != stdx::error{}) {
          std::cerr << "Send failed for " << label << ": " << errorText(err) << "\n";
          return false;
        }
        if (opt.verbose)
          std::cout << "TX RQ1 " << label << " attempt " << (attempt + 1) << ": " << toHex(rq1)
                    << "\n";
        else
          std::cout << "TX RQ1 " << label << "\n";

        std::vector<std::vector<std::uint8_t>> stored;
        const int got = collectGroupPayload(
            inbox, stored, payload, roland::kGroupPayloadBytes, timeout, opt.verbose);
        if (got >= roland::kGroupPayloadBytes) {
          requestMsgs.insert(requestMsgs.end(), stored.begin(), stored.end());
          return true;
        }
        std::cerr << label << " incomplete (" << got << "/" << roland::kGroupPayloadBytes
                  << ") after attempt " << (attempt + 1) << "\n";
      }
      return false;
    };

    if (needA) {
      if (requestGroup(roland::kInternalGroupAAddress, "Group A", payloadA))
        received += roland::kPatchesPerGroup;
      std::this_thread::sleep_for(kInterRequestDelay);
    }
    if (needB) {
      if (requestGroup(roland::kInternalGroupBAddress, "Group B", payloadB))
        received += roland::kPatchesPerGroup;
    }

    auto takePatches = [&](const std::vector<std::uint8_t>& buf, int baseIndex) {
      const int n = static_cast<int>(buf.size() / roland::kPatchStride);
      for (int i = 0; i < n; ++i) {
        const int patch = baseIndex + i;
        if (patch < opt.patchFrom || patch > opt.patchTo)
          continue;
        const auto off = static_cast<std::size_t>(i * roland::kPatchStride);
        const auto len = std::min(static_cast<std::size_t>(roland::kPatchSize), buf.size() - off);
        patches[static_cast<std::size_t>(patch - opt.patchFrom)].assign(
            buf.begin() + static_cast<std::ptrdiff_t>(off),
            buf.begin() + static_cast<std::ptrdiff_t>(off + len));
      }
    };
    takePatches(payloadA, 0);
    takePatches(payloadB, roland::kPatchesPerGroup);

    std::size_t totalBytes = 0;
    {
      std::ofstream out(opt.outFile, std::ios::binary);
      if (!out) {
        std::cerr << "Cannot write " << opt.outFile << "\n";
        return 1;
      }
      for (const auto& msg : requestMsgs) {
        out.write(reinterpret_cast<const char*>(msg.data()), static_cast<std::streamsize>(msg.size()));
        totalBytes += msg.size();
      }
    }

    const auto summaryPath = std::filesystem::path(opt.outFile).replace_extension(".txt");
    int named = 0;
    {
      std::ofstream sum(summaryPath);
      sum << "GP-16 dump summary\n";
      sum << "mode=request\n";
      sum << "device_id=0x" << std::hex << static_cast<int>(opt.deviceId) << std::dec << "\n";
      sum << "sysex_messages=" << requestMsgs.size() << "\n";
      sum << "bytes=" << totalBytes << "\n\n";
      for (int patch = opt.patchFrom; patch <= opt.patchTo; ++patch) {
        const auto& buf = patches[static_cast<std::size_t>(patch - opt.patchFrom)];
        if (buf.size() < static_cast<std::size_t>(roland::kPatchNameOffset + roland::kPatchNameLength)) {
          sum << patch << "\tMISSING\n";
          continue;
        }
        ++named;
        sum << patch << "\t" << roland::extractPatchName(buf) << "\t" << buf.size() << " B\n";
      }
    }

    std::cout << "Wrote " << requestMsgs.size() << " DT1 messages (" << totalBytes << " bytes) to "
              << opt.outFile << "\n";
    std::cout << "Summary: " << summaryPath.string() << " (" << named << " patches)\n";
    if (named == 0)
      return 1;
    if (named < (opt.patchTo - opt.patchFrom + 1)) {
      std::cerr << "Warning: incomplete dump (" << named << "/"
                << (opt.patchTo - opt.patchFrom + 1) << ")\n";
      return 3;
    }
    return 0;
  }

  std::size_t totalBytes = 0;
  int written = 0;
  {
    std::ofstream out(opt.outFile, std::ios::binary);
    if (!out) {
      std::cerr << "Cannot write " << opt.outFile << "\n";
      return 1;
    }
    for (const auto& msg : patches) {
      if (msg.empty())
        continue;
      out.write(reinterpret_cast<const char*>(msg.data()), static_cast<std::streamsize>(msg.size()));
      totalBytes += msg.size();
      ++written;
    }
  }

  const auto summaryPath = std::filesystem::path(opt.outFile).replace_extension(".txt");
  {
    std::ofstream sum(summaryPath);
    sum << "GP-16 dump summary\n";
    sum << "mode=listen\n";
    sum << "device_id=0x" << std::hex << static_cast<int>(opt.deviceId) << std::dec << "\n";
    sum << "patches_written=" << written << "\n";
    sum << "bytes=" << totalBytes << "\n\n";
    for (int patch = opt.patchFrom; patch <= opt.patchTo; ++patch) {
      const auto& msg = patches[static_cast<std::size_t>(patch - opt.patchFrom)];
      if (msg.empty()) {
        sum << patch << "\tMISSING\n";
        continue;
      }
      auto parsed = roland::parseDt1(msg);
      sum << patch << "\t" << extractName(parsed) << "\t" << msg.size() << " B\n";
    }
  }

  std::cout << "Wrote " << written << " patches (" << totalBytes << " bytes) to " << opt.outFile
            << "\n";
  std::cout << "Summary: " << summaryPath.string() << "\n";

  if (written == 0)
    return 1;
  if (written < (opt.patchTo - opt.patchFrom + 1)) {
    std::cerr << "Warning: incomplete dump (" << written << "/"
              << (opt.patchTo - opt.patchFrom + 1) << ")\n";
    return 3;
  }
  return 0;
}

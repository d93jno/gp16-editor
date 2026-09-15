#include "EffectSpecs.h"
#include "Patch.h"
#include "PatchBank.h"
#include "PatchChartParser.h"
#include "RolandSysex.h"

#include <libremidi/libremidi.hpp>

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <condition_variable>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
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
  bool pokeMode = false;
  bool probeInternalWrite = false;
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
  bool decodeMode = false;
  std::string decodeFile;
  std::string importFile;
  std::uint8_t pokeLow = 0;
  std::uint8_t pokeHigh = 100;
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
      << "      --decode <file>    Offline: decode a captured .bin (shape auto-detected) and exit\n"
      << "      --import <file.pch> Offline: parse a legacy .PCH chart into a Patch and decode it\n"
      << "      --poke             Play Mode probe: compressor sustain @ 00 00 11 with/without\n"
      << "                        SOUND CHANGE REQUEST @ 00 00 75 (needs -o)\n"
      << "      --probe-internal-write  Hardware spike: RQ1 then identity DT1 to\n"
      << "                        01 00 63 (A11 OUTPUT CHANNEL). Does not change the value.\n"
      << "  -v, --verbose          Log each message\n"
      << "  -h, --help             Show this help\n"
      << "\n"
      << "Examples:\n"
      << "  " << argv0 << " --list\n"
      << "  " << argv0 << " -i \"USB MIDI\" -f dump.bin\n"
      << "      # then start bulk dump on the GP-16 panel\n"
      << "  " << argv0 << " --request -i \"USB MIDI\" -o \"USB MIDI\" -d 00 -f dump.bin\n"
      << "  " << argv0 << " --decode captures/dump-20260730-153932.bin\n"
      << "  " << argv0 << " --import patches/ACOUSTIC.PCH --decode\n"
      << "  " << argv0 << " --poke -o \"USB MIDI\" -d 00 -v\n"
      << "  " << argv0 << " --probe-internal-write -i \"USB MIDI\" -o \"USB MIDI\" -d 00 -v\n";
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
    } else if (a == "--decode") {
      opt.decodeMode = true;
      if (i + 1 < argc && argv[i + 1][0] != '-')
        opt.decodeFile = need(a.c_str());
    } else if (a == "--import") {
      opt.importFile = need(a.c_str());
    } else if (a == "--poke") {
      opt.pokeMode = true;
    } else if (a == "--probe-internal-write") {
      opt.probeInternalWrite = true;
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

void printDecodeRow(std::ostream& os, int index, const Patch& patch)
{
  os << std::setw(3) << index << "  " << Patch::displayIdFor(index) << "  ";
  if (!patch.isPresent()) {
    os << "MISSING\n";
    return;
  }

  os << std::left << std::setw(18) << patch.name() << std::right;

  os << " A:";
  for (int id : patch.blockAOrder())
    os << ' ' << Patch::effectName(id, patch.blockB2Mode(), patch.isDistortion());
  os << "  B:";
  for (int id : patch.blockBOrder())
    os << ' ' << Patch::effectName(id, patch.blockB2Mode(), patch.isDistortion());

  os << "  on:";
  bool any = false;
  for (int id = 0; id < Patch::kEffectCount; ++id) {
    if (patch.isEffectEnabled(id)) {
      os << (any ? "," : " ") << Patch::effectName(id, patch.blockB2Mode(), patch.isDistortion());
      any = true;
    }
  }
  if (!any)
    os << " (none)";
  os << "\n";
}

int runDecode(const std::string& file)
{
  PatchBank bank;
  std::string error;
  if (!bank.loadFile(file, error)) {
    std::cerr << "Decode failed: " << error << "\n";
    return 1;
  }

  std::cout << "Shape: "
            << (bank.shape() == PatchBank::IngestShape::PanelBulkDump ? "panel bulk dump"
                : bank.shape() == PatchBank::IngestShape::Rq1BulkDump ? "RQ1 bulk dump"
                                                                       : "unknown")
            << "\n";
  std::cout << "Patches present: " << bank.presentCount() << "/" << PatchBank::kPatchCount << "\n\n";

  for (int i = 0; i < PatchBank::kPatchCount; ++i)
    printDecodeRow(std::cout, i, bank.patchAt(i));

  if (bank.presentCount() == 0)
    return 1;
  if (bank.presentCount() < PatchBank::kPatchCount)
    return 3;
  return 0;
}

const char* slotTag(int identity)
{
  static constexpr const char* kTags[] = {"A-1", "A-2", "A-3", "A-4", "A-5", "A-6",
                                          "B-1", "B-2", "B-3", "B-4", "B-5", "B-6"};
  if (identity < 0 || identity >= Patch::kEffectCount)
    return "?";
  return kTags[identity];
}

std::string formatHz(double hz)
{
  std::ostringstream os;
  os << std::fixed;
  if (hz >= 1000.0)
    os << std::setprecision(2) << (hz / 1000.0) << " kHz";
  else
    os << std::setprecision(0) << hz << " Hz";
  return os.str();
}

std::string sequenceDigits(const std::array<int, 6>& order)
{
  std::string s;
  s.resize(6);
  for (int i = 0; i < 6; ++i) {
    const int id = order[static_cast<std::size_t>(i)];
    const int local = id < 6 ? id : id - 6;
    s[static_cast<std::size_t>(i)] = static_cast<char>('1' + local);
  }
  return s;
}

std::string formatParamValue(const ParamSpec& spec, int raw)
{
  if (spec.type == ParamType::Combo && spec.comboItems && spec.comboCount > 0) {
    const int i = std::clamp(raw, spec.min, spec.max);
    if (i >= 0 && i < spec.comboCount)
      return spec.comboItems[i];
  }
  if (spec.type == ParamType::Checkbox)
    return raw ? "ON" : "OFF";
  if ((spec.offset == 0x4F || spec.offset == 0x53) && raw >= spec.max)
    return "THRU";
  if (spec.offset == 0x51) {
    const double sec = raw <= 45 ? 0.5 + static_cast<double>(raw) * 0.1
                                 : 5.5 + static_cast<double>(raw - 46) * 0.5;
    std::ostringstream os;
    os << std::fixed << std::setprecision(1) << sec << " sec";
    return os.str();
  }
  if (spec.offset == 0x4F || spec.offset == 0x53) {
    ParamSpec freq = spec;
    freq.min = 0;
    freq.max = 199;
    freq.transform = DisplayTransform::FreqLog;
    freq.displayMin = 500;
    freq.displayMax = 8000;
    return formatHz(rawToDisplay(freq, raw));
  }
  if (spec.offset == 0x3B && spec.byteWidth >= 2) {
    const int e = raw / 2;
    std::ostringstream os;
    os << "E " << e << " / D " << (100 - e);
    return os.str();
  }

  const double display = rawToDisplay(spec, raw);
  std::ostringstream os;
  os << std::fixed;
  switch (spec.transform) {
    case DisplayTransform::LevelDb:
      os << std::setprecision(1);
      if (display == 0.0)
        os << "0.0 dB";
      else
        os << std::showpos << display << " dB";
      break;
    case DisplayTransform::QValue:
      os << std::setprecision(1) << display;
      break;
    case DisplayTransform::FreqLinear:
    case DisplayTransform::FreqLog:
      return formatHz(display);
    default: {
      const int n = static_cast<int>(std::lround(display));
      os << n;
      if (spec.suffix && std::string_view(spec.suffix).find("ms") != std::string_view::npos)
        os << " msec";
      break;
    }
  }
  return os.str();
}

void printImportedPatch(std::ostream& os, const ParsedChart& chart, const Patch& patch)
{
  printDecodeRow(os, patch.index(), patch);
  os << "\n";
  os << "Name: " << patch.name() << "\n";
  if (!chart.author.empty())
    os << "Author: " << chart.author << "\n";
  if (!chart.comments.empty())
    os << "Comments: " << chart.comments << "\n";
  os << "LCD: " << patch.playModeLcdLine1() << " / " << patch.playModeLcdLine2() << "\n";
  os << "SEQUENCE BLOCK A  " << sequenceDigits(patch.blockAOrder()) << "\n";
  os << "SEQUENCE BLOCK B  " << sequenceDigits(patch.blockBOrder()) << "\n";
  os << "\n";

  for (int id = 0; id < Patch::kEffectCount; ++id) {
    if (!patch.isEffectEnabled(id))
      continue;
    const EffectKind kind = kindForSlot(id, patch.blockB2Mode(), patch.isDistortion());
    const auto& spec = specFor(kind);
    os << slotTag(id) << " " << spec.name << "\n";
    for (const auto& param : spec.params) {
      os << "  ";
      if (param.group)
        os << param.group << " ";
      os << param.label << " " << formatParamValue(param, readParam(patch, param)) << "\n";
    }
    os << "\n";
  }

  const auto globals = allGlobalParams();
  for (const auto& param : globals)
    os << param.label << " " << formatParamValue(param, readParam(patch, param)) << "\n";
}

bool isPatchChartPath(const std::string& path)
{
  auto ext = std::filesystem::path(path).extension().string();
  for (char& c : ext)
    c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
  return ext == ".pch";
}

int runImport(const std::string& file)
{
  std::string error;
  const auto chart = parsePatchChartFile(file, error);
  if (!error.empty()) {
    std::cerr << "Import failed: " << error << "\n";
    return 1;
  }

  const Patch patch = chartToPatch(chart);
  std::cout << "Imported: " << file << "\n";
  if (chart.warnings.empty()) {
    std::cout << "Warnings: (none)\n\n";
  } else {
    std::cout << "Warnings (" << chart.warnings.size() << "):\n";
    for (const auto& w : chart.warnings)
      std::cout << "  " << w << "\n";
    std::cout << "\n";
  }
  printImportedPatch(std::cout, chart, patch);
  return 0;
}

bool sendDt1(
    libremidi::midi_out& out,
    std::uint8_t deviceId,
    std::span<const std::uint8_t, 3> address,
    std::uint8_t value,
    bool verbose,
    const char* label)
{
  const auto msg = roland::buildParameterChange(deviceId, address, value);
  if (auto err = out.send_message(msg.data(), msg.size()); err != stdx::error{}) {
    std::cerr << "Send failed (" << label << "): " << errorText(err) << "\n";
    return false;
  }
  std::cout << "TX " << label << " value=" << static_cast<int>(value);
  if (verbose)
    std::cout << "  " << toHex(msg);
  std::cout << "\n";
  return true;
}

bool sendSysEx(
    libremidi::midi_out& out,
    const std::vector<std::uint8_t>& msg,
    bool verbose,
    const char* label)
{
  if (auto err = out.send_message(msg.data(), msg.size()); err != stdx::error{}) {
    std::cerr << "Send failed (" << label << "): " << errorText(err) << "\n";
    return false;
  }
  std::cout << "TX " << label;
  if (verbose)
    std::cout << "  " << toHex(msg);
  std::cout << "\n";
  return true;
}

std::vector<roland::ParsedDt1> collectDt1(SysexInbox& inbox, std::chrono::milliseconds timeout)
{
  std::vector<roland::ParsedDt1> out;
  auto msgs = inbox.drainFor(timeout);
  for (auto& msg : msgs) {
    auto parsed = roland::parseDt1(msg);
    if (parsed.valid)
      out.push_back(std::move(parsed));
  }
  return out;
}

int runProbeInternalWrite(const Options& opt, libremidi::midi_out& out, SysexInbox& inbox)
{
  constexpr std::array<std::uint8_t, 3> kTempChannel{0x00, 0x00, 0x63};
  constexpr std::array<std::uint8_t, 3> kInternalA11Channel{0x01, 0x00, 0x63};
  constexpr std::array<std::uint8_t, 3> kOneByte{0x00, 0x00, 0x01};
  const auto wait = std::chrono::milliseconds(1500);

  std::cout
      << "\nDirect internal-memory write probe (Phase 4b)\n"
      << "----------------------------------------------\n"
      << "Reads OUTPUT CHANNEL (offset 0x63) from the temporary buffer and from\n"
      << "internal A11 (01 00 63), then writes the same value back (identity DT1).\n"
      << "The stored value is not changed. Device ID 0x" << std::hex
      << static_cast<int>(opt.deviceId) << std::dec << ".\n\n";

  auto rq1 = [&](std::span<const std::uint8_t, 3> address, const char* label) {
    const auto msg = roland::buildRequestData(opt.deviceId, address, kOneByte);
    return sendSysEx(out, msg, opt.verbose, label);
  };
  auto dt1 = [&](std::span<const std::uint8_t, 3> address, std::uint8_t value, const char* label) {
    const auto msg = roland::buildParameterChange(opt.deviceId, address, value);
    return sendSysEx(out, msg, opt.verbose, label);
  };

  if (!rq1(kTempChannel, "RQ1 temp 00 00 63"))
    return 1;
  auto tempHits = collectDt1(inbox, wait);
  std::cout << "  RX " << tempHits.size() << " DT1 after temp RQ1";
  if (!tempHits.empty() && !tempHits.front().data.empty())
    std::cout << ", data[0]=" << static_cast<int>(tempHits.front().data[0]);
  std::cout << "\n";

  std::this_thread::sleep_for(kInterRequestDelay);

  if (!rq1(kInternalA11Channel, "RQ1 internal A11 01 00 63"))
    return 1;
  auto internalHits = collectDt1(inbox, wait);
  std::cout << "  RX " << internalHits.size() << " DT1 after internal RQ1";
  int original = -1;
  if (!internalHits.empty() && !internalHits.front().data.empty()) {
    original = static_cast<int>(internalHits.front().data[0]);
    std::cout << ", data[0]=" << original;
  }
  std::cout << "\n";

  bool wrote = false;
  bool readBack = false;
  int after = -1;
  if (original >= 0) {
    std::this_thread::sleep_for(kInterRequestDelay);
    if (!dt1(kInternalA11Channel, static_cast<std::uint8_t>(original),
             "DT1 identity write 01 00 63"))
      return 1;
    wrote = true;
    std::this_thread::sleep_for(kInterRequestDelay);
    if (!rq1(kInternalA11Channel, "RQ1 internal A11 after identity DT1"))
      return 1;
    auto afterHits = collectDt1(inbox, wait);
    std::cout << "  RX " << afterHits.size() << " DT1 after identity write";
    if (!afterHits.empty() && !afterHits.front().data.empty()) {
      after = static_cast<int>(afterHits.front().data[0]);
      readBack = true;
      std::cout << ", data[0]=" << after;
    }
    std::cout << "\n";
  }

  std::cout << "\nFINDING: ";
  if (original < 0) {
    std::cout
        << "no DT1 reply to RQ1 01 00 63. Direct internal-memory patch write is "
           "UNVERIFIED on this setup (host RQ1 often delivers no SysEx on Linux + "
           "generic USB MIDI). Import uses the temporary-buffer path (4a) only.\n";
    return 3;
  }
  if (!wrote) {
    std::cout << "internal read worked but the identity DT1 was not sent.\n";
    return 1;
  }
  if (!readBack) {
    std::cout
        << "internal A11 OUTPUT CHANNEL reads back, but there was no DT1 reply after "
           "the identity write to 01 00 63. Direct write is NOT confirmed. Import "
           "keeps the temporary-buffer path (4a).\n";
    return 3;
  }
  if (after != original) {
    std::cout
        << "identity DT1 to 01 00 63 did not round-trip (wrote " << original
        << ", read " << after << "). Direct internal-memory write does NOT work as "
           "assumed. Import keeps the temporary-buffer path (4a).\n";
    return 1;
  }
  std::cout
      << "identity DT1 to 01 00 63 round-tripped (value " << original
      << "). A 1-byte internal write at that address family is possible. A full "
         "117-byte patch DT1 to 01 <index> 00 is still NOT the import path until a "
         "full-payload test is done; Import still uses temporary buffer + WRITE (4a).\n";
  return 0;
}

int runPoke(const Options& opt, libremidi::midi_out& out)
{
  constexpr std::array<std::uint8_t, 3> kSustainAddr{0x00, 0x00, 0x11};
  constexpr std::array<std::uint8_t, 3> kSoundChangeAddr{0x00, 0x00, 0x75};
  constexpr auto kListenGap = std::chrono::seconds(4);

  std::cout
      << "\nSOUND CHANGE REQUEST probe (Play Mode)\n"
      << "---------------------------------------\n"
      << "On the GP-16: Play Mode, compressor ON, play a sustained note/chord\n"
      << "through the unit so sustain changes are obvious.\n"
      << "Device ID 0x" << std::hex << static_cast<int>(opt.deviceId) << std::dec
      << "; sustain low=" << static_cast<int>(opt.pokeLow)
      << " high=" << static_cast<int>(opt.pokeHigh)
      << "; 50 ms before any 0x75 poke.\n\n";

  std::cout << "=== Trial A: sustain WITHOUT 0x75 ===\n";
  if (!sendDt1(out, opt.deviceId, kSustainAddr, opt.pokeHigh, opt.verbose, "00 00 11 sustain"))
    return 1;
  std::cout << "  Listen " << kListenGap.count() << "s for an audible sustain change…\n";
  std::this_thread::sleep_for(kListenGap);

  if (!sendDt1(out, opt.deviceId, kSustainAddr, opt.pokeLow, opt.verbose, "00 00 11 sustain"))
    return 1;
  std::cout << "  Listen " << kListenGap.count() << "s…\n";
  std::this_thread::sleep_for(kListenGap);

  std::cout << "\n=== Trial B: sustain WITH 0x75 (50 ms later) ===\n";
  if (!sendDt1(out, opt.deviceId, kSustainAddr, opt.pokeHigh, opt.verbose, "00 00 11 sustain"))
    return 1;
  std::this_thread::sleep_for(kInterRequestDelay);
  if (!sendDt1(out, opt.deviceId, kSoundChangeAddr, 0x00, opt.verbose, "00 00 75 sound-change"))
    return 1;
  std::cout << "  Listen " << kListenGap.count() << "s…\n";
  std::this_thread::sleep_for(kListenGap);

  if (!sendDt1(out, opt.deviceId, kSustainAddr, opt.pokeLow, opt.verbose, "00 00 11 sustain"))
    return 1;
  std::this_thread::sleep_for(kInterRequestDelay);
  if (!sendDt1(out, opt.deviceId, kSoundChangeAddr, 0x00, opt.verbose, "00 00 75 sound-change"))
    return 1;
  std::cout << "  Listen " << kListenGap.count() << "s…\n";
  std::this_thread::sleep_for(kListenGap);

  std::cout
      << "\nProbe TX complete.\n"
      << "Compare Trial A vs Trial B:\n"
      << "  - Audible only in B  → Play Mode needs 0x75\n"
      << "  - Audible in A (and B) → Play Mode does not need 0x75\n"
      << "Record the result in qt/README.md (Phase 4 contract for live edit).\n";
  return 0;
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

  if (!opt.importFile.empty())
    return runImport(opt.importFile);
  if (opt.decodeMode) {
    if (opt.decodeFile.empty()) {
      std::cerr << "--decode requires a file (or use --import <file.pch> --decode)\n";
      return 2;
    }
    if (isPatchChartPath(opt.decodeFile))
      return runImport(opt.decodeFile);
    return runDecode(opt.decodeFile);
  }

  const auto api = resolveApi(opt.api);
  std::cout << "API: " << libremidi::get_api_display_name(api) << "\n";
  if (opt.pokeMode)
    std::cout << "Mode: poke (SOUND CHANGE REQUEST)\n";
  else if (opt.probeInternalWrite)
    std::cout << "Mode: probe internal-memory write (Phase 4b)\n";
  else
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

  const bool needIn = opt.probeInternalWrite || !opt.pokeMode;
  const bool needOut = opt.requestMode || opt.pokeMode || opt.probeInternalWrite;

  if (opt.listOnly || (needIn && opt.inPort.empty())) {
    std::cout << "MIDI inputs (" << inNames.size() << "):\n";
    for (std::size_t i = 0; i < inNames.size(); ++i)
      std::cout << "  [" << i << "] " << inNames[i] << "\n";
    std::cout << "MIDI outputs (" << outNames.size() << "):\n";
    for (std::size_t i = 0; i < outNames.size(); ++i)
      std::cout << "  [" << i << "] " << outNames[i] << "\n";
    if (opt.listOnly)
      return 0;
    if (needIn && opt.inPort.empty()) {
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

  if (needIn && opt.inPort.empty())
    opt.inPort = "USB MIDI";
  if (needOut && opt.outPort.empty())
    opt.outPort = "USB MIDI";

  std::optional<std::size_t> inIdx;
  if (needIn) {
    inIdx = prefer(inNames, opt.inPort);
    if (!inIdx) {
      std::cerr << "Input port not found: " << opt.inPort << "\n";
      return 1;
    }
  }

  std::optional<std::size_t> outIdx;
  if (needOut) {
    outIdx = prefer(outNames, opt.outPort);
    if (!outIdx) {
      std::cerr << "Output port not found: " << opt.outPort << "\n";
      return 1;
    }
  }

  if (inIdx)
    std::cout << "Input:  [" << *inIdx << "] " << inNames[*inIdx] << "\n";
  if (outIdx)
    std::cout << "Output: [" << *outIdx << "] " << outNames[*outIdx] << "\n";
  std::cout << "Device ID: 0x" << std::hex << static_cast<int>(opt.deviceId) << std::dec << "\n";
  if (!opt.pokeMode && !opt.probeInternalWrite)
    std::cout << "File: " << opt.outFile << "\n";

  if (opt.pokeMode && !opt.probeInternalWrite) {
    libremidi::midi_out midiout{
        libremidi::output_configuration{},
        libremidi::midi_out_configuration_for(api),
    };
    if (auto err = midiout.open_port(outputs[*outIdx]); err != stdx::error{}) {
      std::cerr << "Failed to open output: " << errorText(err) << "\n";
      return 1;
    }
    return runPoke(opt, midiout);
  }

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
  if (opt.requestMode || opt.probeInternalWrite) {
    midiout = std::make_unique<libremidi::midi_out>(
        libremidi::output_configuration{},
        libremidi::midi_out_configuration_for(api));
    if (auto err = midiout->open_port(outputs[*outIdx]); err != stdx::error{}) {
      std::cerr << "Failed to open output: " << errorText(err) << "\n";
      return 1;
    }
  }

  if (opt.probeInternalWrite) {
    if (!midiout) {
      std::cerr << "Internal-write probe needs an output port.\n";
      return 2;
    }
    return runProbeInternalWrite(opt, *midiout, inbox);
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

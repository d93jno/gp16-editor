#include "PatchBank.h"

#include "RolandSysex.h"

#include <algorithm>
#include <fstream>
#include <iterator>

int PatchBank::presentCount() const
{
  int n = 0;
  for (const auto& p : patches_)
    if (p.isPresent())
      ++n;
  return n;
}

bool PatchBank::hasPatch(int index) const
{
  if (index < 0 || index >= kPatchCount)
    return false;
  return patches_[static_cast<std::size_t>(index)].isPresent();
}

const Patch& PatchBank::patchAt(int index) const
{
  static const Patch empty;
  if (index < 0 || index >= kPatchCount)
    return empty;
  return patches_[static_cast<std::size_t>(index)];
}

Patch& PatchBank::patchAt(int index)
{
  static Patch empty;
  if (index < 0 || index >= kPatchCount)
    return empty;
  return patches_[static_cast<std::size_t>(index)];
}

std::vector<std::vector<std::uint8_t>> PatchBank::splitSysexMessages(std::span<const std::uint8_t> bytes)
{
  std::vector<std::vector<std::uint8_t>> messages;
  std::vector<std::uint8_t> current;
  bool inSysex = false;
  for (auto b : bytes) {
    if (b == 0xF0) {
      inSysex = true;
      current.clear();
      current.push_back(b);
    } else if (inSysex) {
      current.push_back(b);
      if (b == 0xF7) {
        messages.push_back(std::move(current));
        current.clear();
        inSysex = false;
      }
    }
  }
  return messages;
}

void PatchBank::ingestGroupPayload(std::span<const std::uint8_t> payload, int baseIndex)
{
  if (payload.empty())
    return;
  shape_ = IngestShape::Rq1BulkDump;
  const int n = std::min(
      static_cast<int>(payload.size() / static_cast<std::size_t>(roland::kPatchStride)),
      roland::kPatchesPerGroup);
  for (int i = 0; i < n; ++i) {
    const int patchIndex = baseIndex + i;
    if (patchIndex < 0 || patchIndex >= kPatchCount)
      continue;
    const auto off = static_cast<std::size_t>(i) * static_cast<std::size_t>(roland::kPatchStride);
    const auto len = std::min(static_cast<std::size_t>(roland::kPatchSize), payload.size() - off);
    patches_[static_cast<std::size_t>(patchIndex)].parse(payload.subspan(off, len), patchIndex);
  }
}

void PatchBank::ingestPanelMessage(std::span<const std::uint8_t> sysex)
{
  auto parsed = roland::parseDt1(sysex);
  if (!parsed.valid || parsed.address.size() < 2 || parsed.address[0] != 0x0F)
    return;
  const int idx = parsed.address[1];
  if (idx < 0 || idx >= kPatchCount)
    return;
  patches_[static_cast<std::size_t>(idx)].parse(parsed.data, idx);
  shape_ = IngestShape::PanelBulkDump;
}

bool PatchBank::ingestBytes(std::span<const std::uint8_t> bytes, std::string& error)
{
  const auto messages = splitSysexMessages(bytes);
  if (messages.empty()) {
    error = "no SysEx messages found";
    return false;
  }

  std::vector<std::uint8_t> payloadA;
  std::vector<std::uint8_t> payloadB;
  bool sawPanel = false;
  bool sawRq1 = false;

  for (const auto& msg : messages) {
    const auto parsed = roland::parseDt1(msg);
    if (!parsed.valid || parsed.address.size() < 2)
      continue;

    if (parsed.address[0] == 0x0F) {
      sawPanel = true;
      const int idx = parsed.address[1];
      if (idx >= 0 && idx < kPatchCount)
        patches_[static_cast<std::size_t>(idx)].parse(parsed.data, idx);
    } else if (parsed.address[0] == 0x01) {
      sawRq1 = true;
      payloadA.insert(payloadA.end(), parsed.data.begin(), parsed.data.end());
    } else if (parsed.address[0] == 0x02) {
      sawRq1 = true;
      payloadB.insert(payloadB.end(), parsed.data.begin(), parsed.data.end());
    }
  }

  if (sawPanel && sawRq1) {
    error = "mixed ingest shapes in one file";
    return false;
  }
  if (!sawPanel && !sawRq1) {
    error = "no recognizable DT1 patch data found";
    return false;
  }

  shape_ = sawPanel ? IngestShape::PanelBulkDump : IngestShape::Rq1BulkDump;
  if (sawRq1) {
    ingestGroupPayload(payloadA, 0);
    ingestGroupPayload(payloadB, roland::kPatchesPerGroup);
  }
  return true;
}

bool PatchBank::loadFile(const std::filesystem::path& path, std::string& error)
{
  std::ifstream in(path, std::ios::binary);
  if (!in) {
    error = "cannot open " + path.string();
    return false;
  }
  const std::vector<std::uint8_t> bytes(
      (std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
  return ingestBytes(bytes, error);
}

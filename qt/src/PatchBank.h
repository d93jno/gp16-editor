#pragma once

#include "Patch.h"

#include <array>
#include <cstdint>
#include <filesystem>
#include <span>
#include <string>
#include <vector>

class PatchBank
{
public:
  enum class IngestShape { Unknown, PanelBulkDump, Rq1BulkDump };

  static constexpr int kPatchCount = 128;

  [[nodiscard]] IngestShape shape() const { return shape_; }
  [[nodiscard]] int presentCount() const;
  [[nodiscard]] bool hasPatch(int index) const;
  [[nodiscard]] const Patch& patchAt(int index) const;
  [[nodiscard]] Patch& patchAt(int index);

  // Reads a captured .bin (concatenated raw SysEx messages), auto-detects
  // whether it is a panel bulk dump or an RQ1 bulk dump, and ingests it.
  bool loadFile(const std::filesystem::path& path, std::string& error);

  // Same as loadFile but operating on bytes already in memory.
  bool ingestBytes(std::span<const std::uint8_t> bytes, std::string& error);

  // One panel "Listen" DT1 message (address 0F <idx> 00).
  void ingestPanelMessage(std::span<const std::uint8_t> sysex);

  // One group's concatenated RQ1 data payload (8192 bytes for a full group).
  void ingestGroupPayload(std::span<const std::uint8_t> payload, int baseIndex);

private:
  static std::vector<std::vector<std::uint8_t>> splitSysexMessages(std::span<const std::uint8_t> bytes);

  std::array<Patch, kPatchCount> patches_{};
  IngestShape shape_ = IngestShape::Unknown;
};

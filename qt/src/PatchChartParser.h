#pragma once

#include "EffectSpecs.h"

#include <array>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

struct ParsedField {
  int offset = 0;
  int byteWidth = 1;
  int raw = 0;
  const char* label = nullptr;
  const char* group = nullptr;
};

struct ParsedSlot {
  int identity = 0;
  EffectKind kind = EffectKind::Count;
  bool sawHeader = false;
  bool present = false;
  std::vector<ParsedField> fields;
};

struct ParsedChart {
  std::string name;
  std::string author;
  std::string comments;
  std::string programChangeRaw;
  std::optional<char> programChangeGroup;
  std::optional<int> programChangeNumber;

  std::array<int, 6> blockAOrder{0, 1, 2, 3, 4, 5};
  std::array<int, 6> blockBOrder{6, 7, 8, 9, 10, 11};
  std::array<bool, 12> summaryEnabled{};
  bool sawBlockAOnOff = false;
  bool sawBlockBOnOff = false;

  std::optional<bool> isDistortion;
  std::optional<int> blockB2Mode;

  std::array<ParsedSlot, 12> effectSlots{};

  std::optional<int> masterVolume;
  std::optional<int> outputChannel;

  std::vector<std::string> warnings;
};

[[nodiscard]] const ParsedField* findField(const ParsedSlot& slot, int offset);

[[nodiscard]] ParsedChart parsePatchChart(std::string_view text);
[[nodiscard]] ParsedChart parsePatchChartFile(const std::filesystem::path& path,
                                              std::string& error);

[[nodiscard]] Patch chartToPatch(const ParsedChart& chart, int index = 0);

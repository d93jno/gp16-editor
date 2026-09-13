#pragma once

#include <array>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

class Patch
{
public:
  static constexpr int kEffectCount = 12;

  void parse(std::span<const std::uint8_t> data, int index);

  [[nodiscard]] int index() const { return index_; }
  [[nodiscard]] bool isPresent() const { return present_; }
  [[nodiscard]] const std::string& name() const { return name_; }
  [[nodiscard]] std::string displayId() const { return displayIdFor(index_); }

  [[nodiscard]] const std::array<int, 6>& blockAOrder() const { return blockA_; }
  [[nodiscard]] const std::array<int, 6>& blockBOrder() const { return blockB_; }

  [[nodiscard]] int blockB2Mode() const { return blockB2Mode_; }
  [[nodiscard]] bool isDistortion() const { return isDistortion_; }

  [[nodiscard]] bool isEffectEnabled(int identity) const;

  [[nodiscard]] std::uint8_t byteAt(int offset) const;
  [[nodiscard]] int wordAt(int msbOffset) const;

  [[nodiscard]] std::span<const std::uint8_t> rawData() const { return data_; }

  [[nodiscard]] static std::string displayIdFor(int index);
  [[nodiscard]] static std::string effectName(int identity, int blockB2Mode = 0, bool isDistortion = true);

private:
  int index_ = 0;
  bool present_ = false;
  std::string name_;
  std::array<int, 6> blockA_{0, 1, 2, 3, 4, 5};
  std::array<int, 6> blockB_{6, 7, 8, 9, 10, 11};
  int blockB2Mode_ = 0;
  bool isDistortion_ = true;
  std::uint8_t onOffHigh_ = 0;
  std::uint8_t onOffLow_ = 0;
  std::vector<std::uint8_t> data_;
};

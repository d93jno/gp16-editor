#pragma once

#include <array>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace roland {

inline constexpr std::uint8_t kGp16ModelId = 0x2A;

inline constexpr std::array<std::uint8_t, 3> kInternalGroupAAddress{0x01, 0x00, 0x00};
inline constexpr std::array<std::uint8_t, 3> kInternalGroupBAddress{0x01, 0x40, 0x00};
inline constexpr std::array<std::uint8_t, 3> kInternalGroupSize{0x00, 0x40, 0x00};

inline constexpr int kGroupPayloadBytes = 8192;
inline constexpr int kPatchStride = 0x80;
inline constexpr int kPatchSize = 0x7F;
inline constexpr int kPatchDataBytes = 117;
inline constexpr int kPatchNameOffset = 0x64;
inline constexpr int kPatchNameLength = 16;
inline constexpr int kPatchesPerGroup = 64;

[[nodiscard]] std::uint8_t checksum(std::span<const std::uint8_t> addressAndData);

[[nodiscard]] std::vector<std::uint8_t> buildRequestData(
    std::uint8_t deviceId,
    std::span<const std::uint8_t, 3> address,
    std::span<const std::uint8_t, 3> size);

[[nodiscard]] std::vector<std::uint8_t> buildDataSet(
    std::uint8_t deviceId,
    std::span<const std::uint8_t> address,
    std::span<const std::uint8_t> data);

[[nodiscard]] std::vector<std::uint8_t> buildParameterChange(
    std::uint8_t deviceId,
    std::span<const std::uint8_t, 3> address,
    std::uint8_t value);

struct ParsedDt1 {
  std::uint8_t deviceId = 0;
  std::uint8_t modelId = 0;
  std::vector<std::uint8_t> address;
  std::vector<std::uint8_t> data;
  bool valid = false;
};

[[nodiscard]] ParsedDt1 parseDt1(std::span<const std::uint8_t> sysex);

[[nodiscard]] std::string extractPatchName(std::span<const std::uint8_t> patchPayload);

} // namespace roland

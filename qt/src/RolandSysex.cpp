#include "RolandSysex.h"

#include <numeric>

namespace roland {

std::uint8_t checksum(std::span<const std::uint8_t> addressAndData)
{
  const auto sum = std::accumulate(
      addressAndData.begin(), addressAndData.end(), 0,
      [](int acc, std::uint8_t b) { return acc + static_cast<int>(b); });
  const auto remainder = sum % 128;
  const auto cs = 128 - remainder;
  return static_cast<std::uint8_t>(cs == 128 ? 0 : cs);
}

std::vector<std::uint8_t> buildRequestData(
    std::uint8_t deviceId,
    std::span<const std::uint8_t, 3> address,
    std::span<const std::uint8_t, 3> size)
{
  std::vector<std::uint8_t> payload;
  payload.reserve(6);
  payload.insert(payload.end(), address.begin(), address.end());
  payload.insert(payload.end(), size.begin(), size.end());

  std::vector<std::uint8_t> msg;
  msg.reserve(13);
  msg.push_back(0xF0);
  msg.push_back(0x41);
  msg.push_back(deviceId);
  msg.push_back(kGp16ModelId);
  msg.push_back(0x11);
  msg.insert(msg.end(), payload.begin(), payload.end());
  msg.push_back(checksum(payload));
  msg.push_back(0xF7);
  return msg;
}

std::vector<std::uint8_t> buildDataSet(
    std::uint8_t deviceId,
    std::span<const std::uint8_t> address,
    std::span<const std::uint8_t> data)
{
  std::vector<std::uint8_t> payload;
  payload.reserve(address.size() + data.size());
  payload.insert(payload.end(), address.begin(), address.end());
  payload.insert(payload.end(), data.begin(), data.end());

  std::vector<std::uint8_t> msg;
  msg.reserve(6 + payload.size() + 2);
  msg.push_back(0xF0);
  msg.push_back(0x41);
  msg.push_back(deviceId);
  msg.push_back(kGp16ModelId);
  msg.push_back(0x12);
  msg.insert(msg.end(), payload.begin(), payload.end());
  msg.push_back(checksum(payload));
  msg.push_back(0xF7);
  return msg;
}

std::vector<std::uint8_t> buildParameterChange(
    std::uint8_t deviceId,
    std::span<const std::uint8_t, 3> address,
    std::uint8_t value)
{
  const std::uint8_t data[1] = {value};
  return buildDataSet(deviceId, address, data);
}

ParsedDt1 parseDt1(std::span<const std::uint8_t> sysex)
{
  ParsedDt1 out;
  if (sysex.size() < 10)
    return out;
  if (sysex.front() != 0xF0 || sysex.back() != 0xF7)
    return out;
  if (sysex[1] != 0x41 || sysex[4] != 0x12)
    return out;

  out.deviceId = sysex[2];
  out.modelId = sysex[3];

  constexpr std::size_t kAddrLen = 3;
  const std::size_t bodyEnd = sysex.size() - 2;
  if (bodyEnd < 5 + kAddrLen)
    return out;

  out.address.assign(sysex.begin() + 5, sysex.begin() + 5 + kAddrLen);
  out.data.assign(sysex.begin() + 5 + kAddrLen, sysex.begin() + bodyEnd);
  out.valid = (out.modelId == kGp16ModelId);
  return out;
}

std::string extractPatchName(std::span<const std::uint8_t> patchPayload)
{
  if (static_cast<int>(patchPayload.size()) < kPatchNameOffset + kPatchNameLength)
    return {};
  std::string s;
  s.reserve(kPatchNameLength);
  for (int i = 0; i < kPatchNameLength; ++i) {
    const auto b = patchPayload[static_cast<std::size_t>(kPatchNameOffset + i)];
    s.push_back(b >= 32 && b < 127 ? static_cast<char>(b) : ' ');
  }
  while (!s.empty() && (s.back() == ' ' || s.back() == '\0'))
    s.pop_back();
  return s;
}

} // namespace roland

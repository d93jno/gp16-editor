#include "Patch.h"

#include "RolandSysex.h"

namespace {

// Bit positions of the EFFECT ON/OFF map at offsets 0x0D (high byte) and
// 0x0E (low byte), keyed by effect identity (Table 1: joint data values 0-11).
bool isBitSet(std::uint8_t byteValue, int bit)
{
  return (byteValue & (1u << bit)) != 0;
}

} // namespace

void Patch::parse(std::span<const std::uint8_t> data, int index)
{
  index_ = index;
  data_.assign(data.begin(), data.end());
  present_ = !data_.empty();

  for (int i = 0; i < 5 && i < static_cast<int>(data_.size()); ++i)
    blockA_[static_cast<std::size_t>(i)] = data_[static_cast<std::size_t>(i)];
  blockA_[5] = 5;

  for (int i = 0; i < 5 && (6 + i) < static_cast<int>(data_.size()); ++i)
    blockB_[static_cast<std::size_t>(i)] = data_[static_cast<std::size_t>(6 + i)];
  blockB_[5] = 11;

  if (data_.size() > 0x0C)
    blockB2Mode_ = data_[0x0C] & 0x03;

  if (data_.size() > 0x0D) {
    onOffHigh_ = data_[0x0D];
    isDistortion_ = (onOffHigh_ & 0x40) == 0;
  }
  if (data_.size() > 0x0E)
    onOffLow_ = data_[0x0E];

  name_ = roland::extractPatchName(data_);
}

bool Patch::isEffectEnabled(int identity) const
{
  switch (identity) {
    case 0: return isBitSet(onOffLow_, 0);   // Compressor (A-1)
    case 1: return isBitSet(onOffLow_, 1);   // Distortion/Overdrive (A-2)
    case 2: return isBitSet(onOffLow_, 2);   // Picking Filter (A-3)
    case 3: return isBitSet(onOffLow_, 3);   // Step Phaser (A-4)
    case 4: return isBitSet(onOffLow_, 4);   // Parametric EQ (A-5)
    case 5: return isBitSet(onOffLow_, 5);   // Noise Suppressor (A-6)
    case 6: return isBitSet(onOffLow_, 6);   // Short Delay (B-1)
    case 7: return isBitSet(onOffHigh_, 0);  // Block B-2 (Chorus/Flanger/Pitch/Space-D)
    case 8: return isBitSet(onOffHigh_, 1);  // Auto Panpot (B-3)
    case 9: return isBitSet(onOffHigh_, 2);  // Tap Delay (B-4)
    case 10: return isBitSet(onOffHigh_, 3); // Reverb (B-5)
    case 11: return isBitSet(onOffHigh_, 4); // Lineout Filter (B-6)
    default: return false;
  }
}

std::uint8_t Patch::byteAt(int offset) const
{
  if (offset < 0 || static_cast<std::size_t>(offset) >= data_.size())
    return 0;
  return data_[static_cast<std::size_t>(offset)];
}

int Patch::wordAt(int msbOffset) const
{
  return (static_cast<int>(byteAt(msbOffset)) << 7) | static_cast<int>(byteAt(msbOffset + 1));
}

std::string Patch::displayIdFor(int index)
{
  if (index < 0 || index > 127)
    return "??";
  const char group = index < 64 ? 'A' : 'B';
  const int local = index % 64;
  const int bank = local / 8 + 1;
  const int number = local % 8 + 1;
  return std::string(1, group) + std::to_string(bank) + std::to_string(number);
}

std::string Patch::effectName(int identity, int blockB2Mode, bool isDistortion)
{
  switch (identity) {
    case 0: return "Compressor";
    case 1: return isDistortion ? "Distortion" : "Overdrive";
    case 2: return "Picking Filter";
    case 3: return "Step Phaser";
    case 4: return "Parametric EQ";
    case 5: return "Noise Suppressor";
    case 6: return "Short Delay";
    case 7:
      switch (blockB2Mode) {
        case 0: return "Chorus";
        case 1: return "Flanger";
        case 2: return "Pitch Shifter";
        case 3: return "Space-D";
        default: return "Block B-2";
      }
    case 8: return "Auto Panpot";
    case 9: return "Tap Delay";
    case 10: return "Reverb";
    case 11: return "Lineout Filter";
    default: return "?";
  }
}

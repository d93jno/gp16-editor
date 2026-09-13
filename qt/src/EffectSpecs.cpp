#include "EffectSpecs.h"

#include <algorithm>
#include <cmath>
#include <iterator>

namespace {

constexpr const char* kLow = "Low";
constexpr const char* kLowMid = "Low Mid";
constexpr const char* kHighMid = "High Mid";
constexpr const char* kHigh = "High";
constexpr const char* kOut = "Out";
constexpr const char* kCenter = "Center";
constexpr const char* kLeft = "Left";
constexpr const char* kRight = "Right";
constexpr const char* kMix = "Mix";

constexpr const char* kUpDown[] = {"Up", "Down"};
constexpr const char* kSpaceDMode[] = {"1", "2", "3", "4"};
constexpr const char* kPanMode[] = {"Panpot", "Tremolo"};
constexpr const char* kReverbMode[] = {
    "Room 1", "Room 2", "Room 3", "Hall 1", "Hall 2", "Hall 3",
    "Plate 1", "Plate 2", "Spring 1", "Spring 2"};

constexpr ParamSpec kCompressor[] = {
    {.label = "Tone", .offset = 0x0F, .transform = DisplayTransform::Offset50},
    {.label = "Attack", .offset = 0x10},
    {.label = "Sustain", .offset = 0x11},
    {.label = "Level", .offset = 0x12},
};

constexpr ParamSpec kDistortion[] = {
    {.label = "Tone", .offset = 0x13, .transform = DisplayTransform::Offset50},
    {.label = "Distortion", .offset = 0x14},
    {.label = "Level", .offset = 0x15},
};

constexpr ParamSpec kOverdrive[] = {
    {.label = "Tone", .offset = 0x16, .transform = DisplayTransform::Offset50},
    {.label = "Drive", .offset = 0x17},
    {.label = "Turbo", .offset = 0x18, .max = 1, .type = ParamType::Checkbox},
    {.label = "Level", .offset = 0x19},
};

constexpr ParamSpec kPickingFilter[] = {
    {.label = "Sens", .offset = 0x1A},
    {.label = "Cutoff", .offset = 0x1B},
    {.label = "Q", .offset = 0x1C, .max = 40, .transform = DisplayTransform::QValue},
    {.label = "Up/Down",
     .offset = 0x1D,
     .max = 1,
     .type = ParamType::Combo,
     .comboItems = kUpDown,
     .comboCount = 2},
};

constexpr ParamSpec kStepPhaser[] = {
    {.label = "Rate", .offset = 0x1E},
    {.label = "Depth", .offset = 0x1F},
    {.label = "Manual", .offset = 0x20},
    {.label = "Resonance", .offset = 0x21},
    {.label = "LFO Step", .offset = 0x22},
};

constexpr ParamSpec kParametricEq[] = {
    {.label = "Freq",
     .offset = 0x2B,
     .transform = DisplayTransform::FreqLog,
     .group = kLow,
     .suffix = " Hz",
     .displayMin = 60,
     .displayMax = 250},
    {.label = "Level",
     .offset = 0x2C,
     .max = 48,
     .transform = DisplayTransform::LevelDb,
     .group = kLow,
     .suffix = " dB"},
    {.label = "Freq",
     .offset = 0x28,
     .transform = DisplayTransform::FreqLog,
     .group = kLowMid,
     .suffix = " Hz",
     .displayMin = 125,
     .displayMax = 1000},
    {.label = "Q",
     .offset = 0x29,
     .max = 40,
     .transform = DisplayTransform::QValue,
     .group = kLowMid},
    {.label = "Level",
     .offset = 0x2A,
     .max = 48,
     .transform = DisplayTransform::LevelDb,
     .group = kLowMid,
     .suffix = " dB"},
    {.label = "Freq",
     .offset = 0x25,
     .transform = DisplayTransform::FreqLinear,
     .group = kHighMid,
     .suffix = " Hz",
     .displayMin = 500,
     .displayMax = 4000},
    {.label = "Q",
     .offset = 0x26,
     .max = 40,
     .transform = DisplayTransform::QValue,
     .group = kHighMid},
    {.label = "Level",
     .offset = 0x27,
     .max = 48,
     .transform = DisplayTransform::LevelDb,
     .group = kHighMid,
     .suffix = " dB"},
    {.label = "Freq",
     .offset = 0x23,
     .transform = DisplayTransform::FreqLinear,
     .group = kHigh,
     .suffix = " Hz",
     .displayMin = 2000,
     .displayMax = 8000},
    {.label = "Level",
     .offset = 0x24,
     .max = 48,
     .transform = DisplayTransform::LevelDb,
     .group = kHigh,
     .suffix = " dB"},
    {.label = "Level",
     .offset = 0x2D,
     .max = 48,
     .transform = DisplayTransform::LevelDb,
     .group = kOut,
     .suffix = " dB"},
};

constexpr ParamSpec kNoiseSuppressor[] = {
    {.label = "Sens", .offset = 0x2E},
    {.label = "Release", .offset = 0x2F},
    {.label = "Level", .offset = 0x30},
};

constexpr ParamSpec kShortDelay[] = {
    {.label = "Time", .offset = 0x31},
    {.label = "E.Level", .offset = 0x32},
};

constexpr ParamSpec kChorus[] = {
    {.label = "Pre-Delay", .offset = 0x33, .suffix = " ms"},
    {.label = "Rate", .offset = 0x34},
    {.label = "Depth", .offset = 0x35},
    {.label = "E.Level", .offset = 0x36},
};

constexpr ParamSpec kFlanger[] = {
    {.label = "Rate", .offset = 0x37},
    {.label = "Depth", .offset = 0x38},
    {.label = "Manual", .offset = 0x39},
    {.label = "Resonance", .offset = 0x3A},
};

constexpr ParamSpec kPitchShifter[] = {
    {.label = "Balance", .offset = 0x3B, .byteWidth = 2, .max = 200},
    {.label = "Chromatic",
     .offset = 0x3D,
     .max = 24,
     .transform = DisplayTransform::Offset12},
    {.label = "Fine", .offset = 0x3E, .transform = DisplayTransform::Offset50},
    {.label = "Feedback", .offset = 0x3F},
    {.label = "Pre-Delay", .offset = 0x40, .suffix = " ms"},
};

constexpr ParamSpec kSpaceD[] = {
    {.label = "Mode",
     .offset = 0x41,
     .max = 3,
     .type = ParamType::Combo,
     .comboItems = kSpaceDMode,
     .comboCount = 4},
};

constexpr ParamSpec kAutoPanpot[] = {
    {.label = "Rate", .offset = 0x42},
    {.label = "Depth", .offset = 0x43},
    {.label = "Mode",
     .offset = 0x44,
     .max = 1,
     .type = ParamType::Combo,
     .comboItems = kPanMode,
     .comboCount = 2},
};

constexpr ParamSpec kTapDelay[] = {
    {.label = "Time",
     .offset = 0x45,
     .byteWidth = 2,
     .max = 1200,
     .group = kCenter,
     .suffix = " ms"},
    {.label = "Level", .offset = 0x4B, .group = kCenter},
    {.label = "Time",
     .offset = 0x47,
     .byteWidth = 2,
     .max = 1200,
     .group = kLeft,
     .suffix = " ms"},
    {.label = "Level", .offset = 0x4C, .group = kLeft},
    {.label = "Time",
     .offset = 0x49,
     .byteWidth = 2,
     .max = 1200,
     .group = kRight,
     .suffix = " ms"},
    {.label = "Level", .offset = 0x4D, .group = kRight},
    {.label = "Feedback", .offset = 0x4E, .group = kMix},
    {.label = "Cutoff", .offset = 0x4F, .byteWidth = 2, .max = 200, .group = kMix},
};

constexpr ParamSpec kReverb[] = {
    {.label = "Decay", .offset = 0x51, .max = 75},
    {.label = "Mode",
     .offset = 0x52,
     .max = 9,
     .type = ParamType::Combo,
     .comboItems = kReverbMode,
     .comboCount = 10},
    {.label = "Cutoff", .offset = 0x53, .byteWidth = 2, .max = 200},
    {.label = "Pre-Delay", .offset = 0x55, .suffix = " ms"},
    {.label = "E.Level", .offset = 0x56},
};

constexpr ParamSpec kLineoutFilter[] = {
    {.label = "Presence", .offset = 0x57},
    {.label = "Treble", .offset = 0x58},
    {.label = "Level", .offset = 0x59},
    {.label = "Bass", .offset = 0x5A},
};

constexpr EffectSpec kEffects[] = {
    {EffectKind::Compressor, "Compressor", LayoutKind::Form, kCompressor},
    {EffectKind::Distortion, "Distortion", LayoutKind::Form, kDistortion},
    {EffectKind::Overdrive, "Overdrive", LayoutKind::Form, kOverdrive},
    {EffectKind::PickingFilter, "Picking Filter", LayoutKind::Form, kPickingFilter},
    {EffectKind::StepPhaser, "Step Phaser", LayoutKind::Form, kStepPhaser},
    {EffectKind::ParametricEq, "Parametric EQ", LayoutKind::EqBands, kParametricEq},
    {EffectKind::NoiseSuppressor, "Noise Suppressor", LayoutKind::Form, kNoiseSuppressor},
    {EffectKind::ShortDelay, "Short Delay", LayoutKind::Form, kShortDelay},
    {EffectKind::Chorus, "Chorus", LayoutKind::Form, kChorus},
    {EffectKind::Flanger, "Flanger", LayoutKind::Form, kFlanger},
    {EffectKind::PitchShifter, "Pitch Shifter", LayoutKind::Form, kPitchShifter},
    {EffectKind::SpaceD, "Space-D", LayoutKind::Form, kSpaceD},
    {EffectKind::AutoPanpot, "Auto Panpot", LayoutKind::Form, kAutoPanpot},
    {EffectKind::TapDelay, "Tap Delay", LayoutKind::TapRows, kTapDelay},
    {EffectKind::Reverb, "Reverb", LayoutKind::Form, kReverb},
    {EffectKind::LineoutFilter, "Lineout Filter", LayoutKind::Form, kLineoutFilter},
};

static_assert(std::size(kEffects) == static_cast<std::size_t>(EffectKind::Count));

int clampRaw(const ParamSpec& spec, int raw)
{
  return std::clamp(raw, spec.min, spec.max);
}

double normalized(int raw, const ParamSpec& spec)
{
  if (spec.max <= spec.min)
    return 0.0;
  return static_cast<double>(clampRaw(spec, raw) - spec.min)
         / static_cast<double>(spec.max - spec.min);
}

double mapFreq(const ParamSpec& spec, int raw, bool logarithmic)
{
  const double t = normalized(raw, spec);
  if (spec.displayMin <= 0.0 || spec.displayMax <= 0.0)
    return spec.displayMin;
  if (logarithmic)
    return spec.displayMin * std::pow(spec.displayMax / spec.displayMin, t);
  return spec.displayMin + (spec.displayMax - spec.displayMin) * t;
}

int unmapFreq(const ParamSpec& spec, double hz, bool logarithmic)
{
  if (spec.displayMax <= spec.displayMin || spec.displayMin <= 0.0) {
    return spec.min;
  }
  hz = std::clamp(hz, spec.displayMin, spec.displayMax);
  double t = 0.0;
  if (logarithmic)
    t = std::log(hz / spec.displayMin) / std::log(spec.displayMax / spec.displayMin);
  else
    t = (hz - spec.displayMin) / (spec.displayMax - spec.displayMin);
  return clampRaw(spec, static_cast<int>(std::lround(
                            static_cast<double>(spec.min)
                            + t * static_cast<double>(spec.max - spec.min))));
}

} // namespace

EffectKind kindForSlot(int identity, int blockB2Mode, bool isDistortion)
{
  switch (identity) {
    case 0: return EffectKind::Compressor;
    case 1: return isDistortion ? EffectKind::Distortion : EffectKind::Overdrive;
    case 2: return EffectKind::PickingFilter;
    case 3: return EffectKind::StepPhaser;
    case 4: return EffectKind::ParametricEq;
    case 5: return EffectKind::NoiseSuppressor;
    case 6: return EffectKind::ShortDelay;
    case 7:
      switch (blockB2Mode & 0x03) {
        case 0: return EffectKind::Chorus;
        case 1: return EffectKind::Flanger;
        case 2: return EffectKind::PitchShifter;
        default: return EffectKind::SpaceD;
      }
    case 8: return EffectKind::AutoPanpot;
    case 9: return EffectKind::TapDelay;
    case 10: return EffectKind::Reverb;
    case 11: return EffectKind::LineoutFilter;
    default: return EffectKind::Count;
  }
}

const EffectSpec& specFor(EffectKind kind)
{
  const int i = static_cast<int>(kind);
  if (i < 0 || i >= static_cast<int>(EffectKind::Count))
    return kEffects[0];
  return kEffects[static_cast<std::size_t>(i)];
}

std::span<const EffectSpec> allEffectSpecs()
{
  return kEffects;
}

int readParam(const Patch& patch, const ParamSpec& spec)
{
  const int raw = spec.byteWidth >= 2 ? patch.wordAt(spec.offset)
                                      : static_cast<int>(patch.byteAt(spec.offset));
  return clampRaw(spec, raw);
}

void writeParam(Patch& patch, const ParamSpec& spec, int raw)
{
  raw = clampRaw(spec, raw);
  if (spec.byteWidth >= 2)
    patch.setWordAt(spec.offset, raw);
  else
    patch.setByteAt(spec.offset, static_cast<std::uint8_t>(raw));
}

double rawToDisplay(const ParamSpec& spec, int raw)
{
  raw = clampRaw(spec, raw);
  switch (spec.transform) {
    case DisplayTransform::Offset50: return static_cast<double>(raw - 50);
    case DisplayTransform::Offset12: return static_cast<double>(raw - 12);
    case DisplayTransform::LevelDb: return static_cast<double>(raw) * 0.5 - 12.0;
    case DisplayTransform::QValue: return 1.0 + static_cast<double>(raw) * 0.1;
    case DisplayTransform::FreqLinear: return mapFreq(spec, raw, false);
    case DisplayTransform::FreqLog: return mapFreq(spec, raw, true);
    case DisplayTransform::Raw:
    default: return static_cast<double>(raw);
  }
}

int displayToRaw(const ParamSpec& spec, double display)
{
  int raw = 0;
  switch (spec.transform) {
    case DisplayTransform::Offset50: raw = static_cast<int>(std::lround(display + 50.0)); break;
    case DisplayTransform::Offset12: raw = static_cast<int>(std::lround(display + 12.0)); break;
    case DisplayTransform::LevelDb:
      raw = static_cast<int>(std::lround((display + 12.0) * 2.0));
      break;
    case DisplayTransform::QValue:
      raw = static_cast<int>(std::lround((display - 1.0) * 10.0));
      break;
    case DisplayTransform::FreqLinear: return unmapFreq(spec, display, false);
    case DisplayTransform::FreqLog: return unmapFreq(spec, display, true);
    case DisplayTransform::Raw:
    default: raw = static_cast<int>(std::lround(display)); break;
  }
  return clampRaw(spec, raw);
}

#pragma once

#include "Patch.h"

#include <cstdint>
#include <span>

enum class ParamType : std::uint8_t { Slider, Checkbox, Combo };

enum class DisplayTransform : std::uint8_t {
  Raw,
  Offset50,
  Offset12,
  LevelDb,
  QValue,
  FreqLinear,
  FreqLog,
};

enum class LayoutKind : std::uint8_t { Form, EqBands, TapRows };

enum class EffectKind : int {
  Compressor = 0,
  Distortion,
  Overdrive,
  PickingFilter,
  StepPhaser,
  ParametricEq,
  NoiseSuppressor,
  ShortDelay,
  Chorus,
  Flanger,
  PitchShifter,
  SpaceD,
  AutoPanpot,
  TapDelay,
  Reverb,
  LineoutFilter,
  Count
};

struct ParamSpec {
  const char* label = nullptr;
  int offset = 0;
  int byteWidth = 1;
  int min = 0;
  int max = 100;
  ParamType type = ParamType::Slider;
  DisplayTransform transform = DisplayTransform::Raw;
  const char* group = nullptr;
  const char* suffix = nullptr;
  double displayMin = 0;
  double displayMax = 0;
  const char* const* comboItems = nullptr;
  int comboCount = 0;
};

struct EffectSpec {
  EffectKind kind = EffectKind::Compressor;
  const char* name = "";
  LayoutKind layout = LayoutKind::Form;
  std::span<const ParamSpec> params{};
};

[[nodiscard]] EffectKind kindForSlot(int identity, int blockB2Mode, bool isDistortion);
[[nodiscard]] const EffectSpec& specFor(EffectKind kind);
[[nodiscard]] std::span<const EffectSpec> allEffectSpecs();
[[nodiscard]] std::span<const ParamSpec> allGlobalParams();

[[nodiscard]] int readParam(const Patch& patch, const ParamSpec& spec);
void writeParam(Patch& patch, const ParamSpec& spec, int raw);

[[nodiscard]] double rawToDisplay(const ParamSpec& spec, int raw);
[[nodiscard]] int displayToRaw(const ParamSpec& spec, double display);

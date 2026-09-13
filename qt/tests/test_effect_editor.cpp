#include "EffectEditor.h"
#include "EffectSpecs.h"
#include "Patch.h"
#include "PatchBank.h"

#include <QApplication>
#include <QByteArray>

#include <cmath>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

namespace {

int failures = 0;

void check(bool condition, const std::string& what)
{
  if (!condition) {
    std::cerr << "FAIL: " << what << "\n";
    ++failures;
  }
}

template <typename T>
void checkEqual(const T& actual, const T& expected, const std::string& what)
{
  if (!(actual == expected)) {
    std::cerr << "FAIL: " << what << " actual=" << actual << " expected=" << expected << "\n";
    ++failures;
  }
}

std::filesystem::path repoRoot()
{
  return std::filesystem::path(GP16_TEST_REPO_ROOT);
}

int identityFor(EffectKind kind)
{
  switch (kind) {
    case EffectKind::Compressor: return 0;
    case EffectKind::Distortion:
    case EffectKind::Overdrive: return 1;
    case EffectKind::PickingFilter: return 2;
    case EffectKind::StepPhaser: return 3;
    case EffectKind::ParametricEq: return 4;
    case EffectKind::NoiseSuppressor: return 5;
    case EffectKind::ShortDelay: return 6;
    case EffectKind::Chorus:
    case EffectKind::Flanger:
    case EffectKind::PitchShifter:
    case EffectKind::SpaceD: return 7;
    case EffectKind::AutoPanpot: return 8;
    case EffectKind::TapDelay: return 9;
    case EffectKind::Reverb: return 10;
    case EffectKind::LineoutFilter: return 11;
    case EffectKind::Count: return -1;
  }
  return -1;
}

void applyKind(Patch& patch, EffectKind kind)
{
  if (kind == EffectKind::Overdrive)
    patch.setByteAt(0x0D, static_cast<std::uint8_t>(patch.byteAt(0x0D) | 0x40));
  else if (kind == EffectKind::Distortion)
    patch.setByteAt(0x0D, static_cast<std::uint8_t>(patch.byteAt(0x0D) & ~0x40));

  int mode = patch.byteAt(0x0C) & ~0x03;
  switch (kind) {
    case EffectKind::Chorus: mode |= 0; break;
    case EffectKind::Flanger: mode |= 1; break;
    case EffectKind::PitchShifter: mode |= 2; break;
    case EffectKind::SpaceD: mode |= 3; break;
    default: break;
  }
  if (identityFor(kind) == 7)
    patch.setByteAt(0x0C, static_cast<std::uint8_t>(mode));
}

void showKind(EffectEditor& editor, Patch& patch, EffectKind kind)
{
  applyKind(patch, kind);
  editor.setPatch(&patch);
  editor.setIdentity(identityFor(kind));
}

void checkPageMatchesPatch(EffectEditor& editor, const Patch& patch, const std::string& label)
{
  const auto& spec = specFor(editor.currentKind());
  checkEqual(editor.paramCount(), static_cast<int>(spec.params.size()),
             label + " param count");
  check(editor.titleText().toStdString() == spec.name, label + " title is " + spec.name);
  for (int i = 0; i < editor.paramCount(); ++i) {
    const auto& param = spec.params[static_cast<std::size_t>(i)];
    checkEqual(editor.paramOffset(i), param.offset, label + " offset " + param.label);
    checkEqual(editor.paramRawValue(i), readParam(patch, param),
               label + " value " + param.label);
  }
}

void testSpecsTable()
{
  checkEqual(static_cast<int>(allEffectSpecs().size()),
             static_cast<int>(EffectKind::Count),
             "16 effect specs (Distortion and Overdrive are separate pages)");

  int twoByte = 0;
  for (const auto& spec : allEffectSpecs()) {
    check(!spec.params.empty(), std::string(spec.name) + " has parameters");
    checkEqual(static_cast<int>(spec.kind), static_cast<int>(specFor(spec.kind).kind),
               std::string(spec.name) + " specFor index");
    for (const auto& param : spec.params) {
      check(param.byteWidth == 1 || param.byteWidth == 2,
            std::string(spec.name) + " " + param.label + " byteWidth");
      if (param.byteWidth == 2)
        ++twoByte;
      if (param.type == ParamType::Combo)
        check(param.comboCount == param.max - param.min + 1,
              std::string(spec.name) + " " + param.label + " combo count");
    }
  }

  // Pitch balance, three tap times, tap cutoff, reverb cutoff.
  checkEqual(twoByte, 6, "six MSB/LSB parameters");

  check(kindForSlot(0, 0, true) == EffectKind::Compressor, "identity 0 is compressor");
  check(kindForSlot(1, 0, true) == EffectKind::Distortion, "A-2 distortion");
  check(kindForSlot(1, 0, false) == EffectKind::Overdrive, "A-2 overdrive");
  check(kindForSlot(7, 0, true) == EffectKind::Chorus, "B-2 chorus");
  check(kindForSlot(7, 1, true) == EffectKind::Flanger, "B-2 flanger");
  check(kindForSlot(7, 2, true) == EffectKind::PitchShifter, "B-2 pitch");
  check(kindForSlot(7, 3, true) == EffectKind::SpaceD, "B-2 space-d");
  check(kindForSlot(11, 0, true) == EffectKind::LineoutFilter, "identity 11 lineout");

  checkEqual(displayToRaw(specFor(EffectKind::Compressor).params[0], 0.0), 50,
             "tone display 0 -> raw 50");
  checkEqual(static_cast<int>(rawToDisplay(specFor(EffectKind::Compressor).params[0], 70)), 20,
             "tone raw 70 -> display 20");
  checkEqual(displayToRaw(specFor(EffectKind::PitchShifter).params[1], 0.0), 12,
             "chromatic display 0 -> raw 12");
  const auto& eqLevel = specFor(EffectKind::ParametricEq).params[1];
  checkEqual(displayToRaw(eqLevel, 0.0), 24, "EQ 0 dB -> raw 24");
  checkEqual(displayToRaw(eqLevel, -12.0), 0, "EQ -12 dB -> raw 0");
  checkEqual(displayToRaw(eqLevel, 12.0), 48, "EQ +12 dB -> raw 48");
  const auto& q = specFor(EffectKind::PickingFilter).params[2];
  checkEqual(displayToRaw(q, 1.0), 0, "Q 1.0 -> raw 0");
  checkEqual(displayToRaw(q, 5.0), 40, "Q 5.0 -> raw 40");
  checkEqual(displayToRaw(q, 3.0), 20, "Q 3.0 -> raw 20");

  const auto& eq = specFor(EffectKind::ParametricEq);
  const auto& loFreq = eq.params[0];
  const auto& lmFreq = eq.params[2];
  const auto& hmFreq = eq.params[5];
  const auto& hiFreq = eq.params[8];
  check(loFreq.transform == DisplayTransform::FreqLog, "low freq is logarithmic");
  check(lmFreq.transform == DisplayTransform::FreqLog, "low-mid freq is logarithmic");
  check(hmFreq.transform == DisplayTransform::FreqLinear, "high-mid freq is linear");
  check(hiFreq.transform == DisplayTransform::FreqLinear, "high freq is linear");
  checkEqual(static_cast<int>(std::lround(rawToDisplay(loFreq, 0))), 60, "low freq raw 0 -> 60 Hz");
  checkEqual(static_cast<int>(std::lround(rawToDisplay(loFreq, 100))), 250,
             "low freq raw 100 -> 250 Hz");
  checkEqual(static_cast<int>(std::lround(rawToDisplay(lmFreq, 0))), 125,
             "low-mid freq raw 0 -> 125 Hz");
  checkEqual(static_cast<int>(std::lround(rawToDisplay(lmFreq, 100))), 1000,
             "low-mid freq raw 100 -> 1000 Hz");
  checkEqual(static_cast<int>(std::lround(rawToDisplay(hmFreq, 0))), 500,
             "high-mid freq raw 0 -> 500 Hz");
  checkEqual(static_cast<int>(std::lround(rawToDisplay(hmFreq, 100))), 4000,
             "high-mid freq raw 100 -> 4000 Hz");
  checkEqual(static_cast<int>(std::lround(rawToDisplay(hiFreq, 0))), 2000,
             "high freq raw 0 -> 2000 Hz");
  checkEqual(static_cast<int>(std::lround(rawToDisplay(hiFreq, 50))), 5000,
             "high freq raw 50 -> 5000 Hz");
  checkEqual(static_cast<int>(std::lround(rawToDisplay(hiFreq, 100))), 8000,
             "high freq raw 100 -> 8000 Hz");
  for (const auto* freq : {&loFreq, &lmFreq, &hmFreq, &hiFreq}) {
    for (int raw = 0; raw <= 100; ++raw) {
      const double hz = rawToDisplay(*freq, raw);
      checkEqual(displayToRaw(*freq, hz), raw,
                 std::string(freq->group) + " freq roundtrip raw " + std::to_string(raw));
    }
  }
}

void testAllPagesRender(EffectEditor& editor)
{
  std::vector<std::uint8_t> data(0x60, 0);
  Patch patch;
  patch.parse(data, 0);

  for (const auto& spec : allEffectSpecs()) {
    showKind(editor, patch, spec.kind);
    check(editor.currentKind() == spec.kind, std::string(spec.name) + " page shown");
    checkEqual(editor.paramCount(), static_cast<int>(spec.params.size()),
               std::string(spec.name) + " renders every parameter");
  }
}

void testCaptureValues(EffectEditor& editor)
{
  PatchBank bank;
  std::string error;
  check(bank.loadFile(repoRoot() / "captures" / "dump-20260730-153932.bin", error),
        "load dump-20260730-153932.bin: " + error);

  int first = -1;
  int second = -1;
  for (int i = 0; i < PatchBank::kPatchCount; ++i) {
    if (!bank.hasPatch(i))
      continue;
    if (first < 0) {
      first = i;
      continue;
    }
    if (bank.patchAt(i).byteAt(0x11) != bank.patchAt(first).byteAt(0x11)
        || bank.patchAt(i).blockB2Mode() != bank.patchAt(first).blockB2Mode()
        || bank.patchAt(i).isDistortion() != bank.patchAt(first).isDistortion()) {
      second = i;
      break;
    }
  }
  check(first >= 0, "capture has a present patch");
  check(second >= 0, "capture has a second patch with different compressor/B-2 data");

  if (first < 0)
    return;

  auto& patchA = bank.patchAt(first);
  editor.setPatch(&patchA);
  for (int identity = 0; identity < Patch::kEffectCount; ++identity) {
    editor.setIdentity(identity);
    checkPageMatchesPatch(editor, patchA,
                          "patch " + std::to_string(first) + " identity " + std::to_string(identity));
  }

  if (second < 0)
    return;

  auto& patchB = bank.patchAt(second);
  editor.setPatch(&patchB);
  editor.setIdentity(0);
  checkPageMatchesPatch(editor, patchB, "switched to patch " + std::to_string(second) + " compressor");
  check(editor.paramRawValue(2) == static_cast<int>(patchB.byteAt(0x11)),
        "compressor sustain is the new patch, not stale");
  check(editor.paramRawValue(2) != static_cast<int>(patchA.byteAt(0x11))
            || patchA.byteAt(0x11) == patchB.byteAt(0x11),
        "sustain changed across the two patches or they happened to match");
}

void testLocalModelWrite(EffectEditor& editor)
{
  std::vector<std::uint8_t> data(0x60, 0);
  data[0x11] = 10;
  data[0x45] = 0;
  data[0x46] = 0;
  Patch patch;
  patch.parse(data, 0);

  editor.setPatch(&patch);
  editor.setIdentity(0);
  editor.setParamRawValue(2, 77);
  checkEqual(static_cast<int>(patch.byteAt(0x11)), 77, "slider write updates compressor sustain");

  applyKind(patch, EffectKind::TapDelay);
  editor.setPatch(&patch);
  editor.setIdentity(9);
  editor.setParamRawValue(0, 1200);
  checkEqual(patch.wordAt(0x45), 1200, "MSB/LSB write updates center tap");
  checkEqual(static_cast<int>(patch.byteAt(0x45)), 1200 >> 7, "center tap MSB");
  checkEqual(static_cast<int>(patch.byteAt(0x46)), 1200 & 0x7F, "center tap LSB");
}

} // namespace

int main(int argc, char* argv[])
{
  qputenv("QT_QPA_PLATFORM", QByteArrayLiteral("offscreen"));
  QApplication app(argc, argv);

  testSpecsTable();

  EffectEditor editor;
  editor.show();

  check(editor.paramCount() == 0, "empty editor has no params");
  check(editor.titleText().isEmpty(), "empty editor has no title");

  testAllPagesRender(editor);
  testCaptureValues(editor);
  testLocalModelWrite(editor);

  if (failures == 0) {
    std::cout << "All effect editor tests passed.\n";
    return 0;
  }
  std::cerr << failures << " effect editor test(s) failed.\n";
  return 1;
}

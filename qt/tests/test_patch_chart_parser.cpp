#include "EffectSpecs.h"
#include "PatchChartParser.h"

#include <array>
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
    std::cerr << "FAIL: " << what << "\n";
    ++failures;
  }
}

void checkEqual(int actual, int expected, const std::string& what)
{
  if (actual != expected) {
    std::cerr << "FAIL: " << what << " actual=" << actual << " expected=" << expected << "\n";
    ++failures;
  }
}

void checkEqual(const std::string& actual, const std::string& expected, const std::string& what)
{
  if (actual != expected) {
    std::cerr << "FAIL: " << what << " actual=\"" << actual << "\" expected=\"" << expected
              << "\"\n";
    ++failures;
  }
}

std::filesystem::path repoRoot()
{
  return std::filesystem::path(GP16_TEST_REPO_ROOT);
}

ParsedChart loadChart(const char* filename)
{
  std::string error;
  auto chart = parsePatchChartFile(repoRoot() / "patches" / filename, error);
  check(error.empty(), std::string(filename) + " opened: " + error);
  return chart;
}

bool warningContains(const ParsedChart& chart, const char* needle)
{
  for (const auto& w : chart.warnings) {
    if (w.find(needle) != std::string::npos)
      return true;
  }
  return false;
}

void dumpWarnings(const ParsedChart& chart, const char* filename)
{
  for (const auto& w : chart.warnings)
    std::cerr << "  " << filename << " warning: " << w << "\n";
}

int fieldRaw(const ParsedSlot& slot, int offset)
{
  const auto* f = findField(slot, offset);
  return f ? f->raw : -9999;
}

void expectRaw(const ParsedSlot& slot, int offset, int expected, const std::string& what)
{
  check(slot.present, what + " slot present");
  checkEqual(fieldRaw(slot, offset), expected, what);
}

int specRaw(EffectKind kind, const char* label, double display, const char* group = nullptr)
{
  const auto& spec = specFor(kind);
  for (const auto& p : spec.params) {
    if (std::string_view(p.label) != label)
      continue;
    if (group == nullptr && p.group == nullptr)
      return displayToRaw(p, display);
    if (group && p.group && std::string_view(p.group) == group)
      return displayToRaw(p, display);
  }
  return -1;
}

int eqRaw(const char* group, const char* label, double display)
{
  return specRaw(EffectKind::ParametricEq, label, display, group);
}

void expectOn(const ParsedChart& chart, int identity, bool on, const std::string& what)
{
  checkEqual(chart.summaryEnabled[static_cast<std::size_t>(identity)], on, what);
}

void testAllPchParse()
{
  int count = 0;
  for (const auto& entry : std::filesystem::directory_iterator(repoRoot() / "patches")) {
    if (entry.path().extension() != ".PCH")
      continue;
    ++count;
    std::string error;
    auto chart = parsePatchChartFile(entry.path(), error);
    const auto name = entry.path().filename().string();
    check(error.empty(), name + " opened: " + error);
    check(!chart.name.empty(), name + " has a patch name");
    if (!error.empty())
      continue;
    std::cout << name << "  name=\"" << chart.name << "\"  warnings=" << chart.warnings.size()
              << "\n";
    dumpWarnings(chart, name.c_str());
  }
  checkEqual(count, 13, "13 .PCH files in patches/");
}

void testAcoustic()
{
  auto chart = loadChart("ACOUSTIC.PCH");
  checkEqual(chart.name, std::string("Acoustic"), "ACOUSTIC name");
  checkEqual(chart.author, std::string("Jonas Nordin"), "ACOUSTIC author");
  checkEqual(chart.comments, std::string("Made for steel-stringed acoustic guitars"),
             "ACOUSTIC comments");
  check(chart.programChangeGroup && *chart.programChangeGroup == 'B', "ACOUSTIC PC group B");
  check(chart.programChangeNumber && *chart.programChangeNumber == 28, "ACOUSTIC PC 28");

  checkEqual(chart.blockAOrder, (std::array<int, 6>{0, 1, 2, 3, 4, 5}), "ACOUSTIC seq A");
  checkEqual(chart.blockBOrder, (std::array<int, 6>{6, 7, 8, 9, 10, 11}), "ACOUSTIC seq B");

  expectOn(chart, 0, true, "A-1 on");
  expectOn(chart, 1, false, "A-2 off");
  expectOn(chart, 4, true, "A-5 on");
  expectOn(chart, 5, true, "A-6 on");
  expectOn(chart, 7, true, "B-2 on");
  expectOn(chart, 10, true, "B-5 on");
  expectOn(chart, 11, true, "B-6 on");

  check(chart.effectSlots[0].present, "A-1 present");
  check(!chart.effectSlots[1].present, "A-2 absent");
  check(chart.effectSlots[4].present, "A-5 present");
  check(chart.effectSlots[7].kind == EffectKind::SpaceD, "B-2 is Space-D");
  check(chart.blockB2Mode && *chart.blockB2Mode == 3, "B-2 mode Space-D");

  expectRaw(chart.effectSlots[0], 0x0F, specRaw(EffectKind::Compressor, "Tone", 0), "TONE 0");
  expectRaw(chart.effectSlots[0], 0x10, 20, "ATTACK 20");
  expectRaw(chart.effectSlots[0], 0x11, 80, "SUSTAIN 80");
  expectRaw(chart.effectSlots[0], 0x12, 40, "LEVEL 40");

  expectRaw(chart.effectSlots[4], 0x23, eqRaw("High", "Freq", 8000), "HI FREQ 8.00 kHz");
  expectRaw(chart.effectSlots[4], 0x24, eqRaw("High", "Level", 11.0), "HI LEVEL +11.0 dB");
  expectRaw(chart.effectSlots[4], 0x25, eqRaw("High Mid", "Freq", 2980), "H.M.FREQ 2.98 kHz");
  expectRaw(chart.effectSlots[4], 0x26, eqRaw("High Mid", "Q", 1.0), "H.M. Q 1.0");
  expectRaw(chart.effectSlots[4], 0x27, eqRaw("High Mid", "Level", 0.5), "H.M. LEVEL +0.5 dB");
  expectRaw(chart.effectSlots[4], 0x28, eqRaw("Low Mid", "Freq", 535), "L.M. FREQ 535 Hz");
  expectRaw(chart.effectSlots[4], 0x29, eqRaw("Low Mid", "Q", 1.0), "L.M. Q 1.0");
  expectRaw(chart.effectSlots[4], 0x2A, eqRaw("Low Mid", "Level", 3.5), "L.M. LEVEL +3.5 dB");
  expectRaw(chart.effectSlots[4], 0x2B, eqRaw("Low", "Freq", 250), "LO FREQ 250 Hz");
  expectRaw(chart.effectSlots[4], 0x2C, eqRaw("Low", "Level", 1.5), "LO LEVEL +1.5 dB");
  expectRaw(chart.effectSlots[4], 0x2D, eqRaw("Out", "Level", 5.5), "OUT LEVEL +5.5 dB");

  expectRaw(chart.effectSlots[5], 0x2E, 32, "NS SENS 32");
  expectRaw(chart.effectSlots[5], 0x2F, 0, "NS RELEASE 0");
  expectRaw(chart.effectSlots[5], 0x30, 100, "NS LEVEL 100");

  expectRaw(chart.effectSlots[7], 0x41, 0, "SPACE-D MODE 1");

  expectRaw(chart.effectSlots[10], 0x51, 5, "DECAY 1.0 sec");
  expectRaw(chart.effectSlots[10], 0x52, 0, "MODE ROOM 1");
  expectRaw(chart.effectSlots[10], 0x53, 200, "CUTOFF THRU");
  expectRaw(chart.effectSlots[10], 0x55, 49, "PRE DELAY 49 msec");
  expectRaw(chart.effectSlots[10], 0x56, 38, "E. LEVEL 38");

  expectRaw(chart.effectSlots[11], 0x57, 10, "PRESENCE 10");
  expectRaw(chart.effectSlots[11], 0x58, 50, "TREBLE 50");
  expectRaw(chart.effectSlots[11], 0x59, 82, "MIDDLE 82");
  expectRaw(chart.effectSlots[11], 0x5A, 78, "BASS 78");

  check(chart.masterVolume && *chart.masterVolume == 75, "MASTER VOLUME 75");
  check(chart.outputChannel && *chart.outputChannel == 0, "CHANNEL 1");
  check(chart.warnings.empty(), "ACOUSTIC has no warnings");
  if (!chart.warnings.empty())
    dumpWarnings(chart, "ACOUSTIC.PCH");
}

void testKnopfler()
{
  auto chart = loadChart("KNOPFLER.PCH");
  checkEqual(chart.name, std::string("Knopfler sound"), "KNOPFLER name");
  check(chart.programChangeNumber && *chart.programChangeNumber == 51, "KNOPFLER PC 51");
  check(chart.isDistortion && *chart.isDistortion, "KNOPFLER A-2 distortion");
  check(chart.effectSlots[1].kind == EffectKind::Distortion, "A-2 Distortion kind");
  expectOn(chart, 0, true, "KNOPFLER A-1 on");
  expectOn(chart, 1, true, "KNOPFLER A-2 on");
  expectOn(chart, 11, true, "KNOPFLER B-6 on");
  check(!chart.effectSlots[7].present, "KNOPFLER no B-2 section");

  expectRaw(chart.effectSlots[1], 0x13, specRaw(EffectKind::Distortion, "Tone", 0), "DIST TONE 0");
  expectRaw(chart.effectSlots[1], 0x14, 31, "DISTORTION 31");
  expectRaw(chart.effectSlots[1], 0x15, 100, "DIST LEVEL 100");

  expectRaw(chart.effectSlots[4], 0x23, eqRaw("High", "Freq", 8000), "KNOPFLER HI FREQ");
  expectRaw(chart.effectSlots[4], 0x24, eqRaw("High", "Level", 0.0), "HI LEVEL 0.0 dB");
  expectRaw(chart.effectSlots[4], 0x25, eqRaw("High Mid", "Freq", 1700), "H.M.FREQ 1.70 kHz");
  expectRaw(chart.effectSlots[4], 0x27, eqRaw("High Mid", "Level", 7.5), "H.M. LEVEL 7.5 dB");
  expectRaw(chart.effectSlots[4], 0x28, eqRaw("Low Mid", "Freq", 1000), "L.M. FREQ 1.00 kHz");
  expectRaw(chart.effectSlots[4], 0x2B, eqRaw("Low", "Freq", 60), "LO FREQ 60 Hz");
  expectRaw(chart.effectSlots[4], 0x2C, eqRaw("Low", "Level", 2.0), "LO LEVEL 2.0 dB");
  expectRaw(chart.effectSlots[4], 0x2D, eqRaw("Out", "Level", 5.5), "OUT LEVEL +5.5 dB");

  expectRaw(chart.effectSlots[5], 0x2E, 28, "NS SENS 28");
  expectRaw(chart.effectSlots[5], 0x30, 52, "NS LEVEL 52");
  expectRaw(chart.effectSlots[11], 0x57, 25, "PRESENCE 25");
  expectRaw(chart.effectSlots[11], 0x59, 100, "MIDDLE 100");
  check(chart.masterVolume && *chart.masterVolume == 71, "MASTER VOLUME 71");
  check(chart.outputChannel && *chart.outputChannel == 0, "CHANNEL 1");
  check(chart.warnings.empty(), "KNOPFLER has no warnings");
  if (!chart.warnings.empty())
    dumpWarnings(chart, "KNOPFLER.PCH");
}

void testBtt70s()
{
  auto chart = loadChart("BTT70S.PCH");
  checkEqual(chart.name, std::string("Back to the 70's"), "BTT70S name");
  check(chart.programChangeNumber && *chart.programChangeNumber == 21, "BTT70S PC 21");
  checkEqual(chart.blockAOrder, (std::array<int, 6>{0, 2, 1, 4, 3, 5}), "BTT70S seq A 132546");

  expectOn(chart, 0, true, "BTT70S compressor on");
  expectOn(chart, 1, true, "BTT70S distortion on");
  expectOn(chart, 2, true, "BTT70S picking filter on");
  expectOn(chart, 3, false, "BTT70S step phaser off");
  expectOn(chart, 4, true, "BTT70S EQ on");
  expectOn(chart, 5, true, "BTT70S NS flagged on");
  expectOn(chart, 10, true, "BTT70S reverb on");
  expectOn(chart, 11, false, "BTT70S lineout off");

  check(chart.effectSlots[3].sawHeader, "A-4 header printed");
  check(!chart.effectSlots[3].present, "A-4 has no values");
  check(!chart.effectSlots[5].present, "A-6 section is missing");
  check(chart.effectSlots[5].sawHeader == false, "A-6 has no header");

  check(warningContains(chart, "A-4"), "warning for empty A-4 section");
  check(warningContains(chart, "A-6"), "warning for NS on with no section");
  check(warningContains(chart, "expression pedal"), "warning that expression is not imported");

  expectRaw(chart.effectSlots[1], 0x14, 41, "DISTORTION 41");
  expectRaw(chart.effectSlots[2], 0x1A, 22, "PF SENS 22");
  expectRaw(chart.effectSlots[2], 0x1B, 25, "CUTOFF FREQ 25");
  expectRaw(chart.effectSlots[2], 0x1C, specRaw(EffectKind::PickingFilter, "Q", 4.5), "Q 4.5");
  expectRaw(chart.effectSlots[2], 0x1D, 0, "UP/DOWN UP");

  expectRaw(chart.effectSlots[4], 0x23, eqRaw("High", "Freq", 6060), "HI FREQ 6.06 kHz");
  expectRaw(chart.effectSlots[4], 0x28, eqRaw("Low Mid", "Freq", 1000), "L.M. FREQ 1.00 kHz");
  expectRaw(chart.effectSlots[4], 0x29, eqRaw("Low Mid", "Q", 1.9), "L.M. Q 1.9");
  expectRaw(chart.effectSlots[4], 0x2C, eqRaw("Low", "Level", 7.0), "LO LEVEL +7.0 dB");
  expectRaw(chart.effectSlots[4], 0x2D, eqRaw("Out", "Level", 6.0), "OUT LEVEL +6.0 dB");

  expectRaw(chart.effectSlots[10], 0x51, 19, "DECAY 2.4 sec");
  expectRaw(chart.effectSlots[10], 0x52, 9, "MODE SPRING 2");
  expectRaw(chart.effectSlots[10], 0x53, 200, "CUTOFF THRU");
  expectRaw(chart.effectSlots[10], 0x55, 25, "PRE DELAY 25 msec");
  expectRaw(chart.effectSlots[10], 0x56, 18, "E. LEVEL 18");

  check(chart.masterVolume && *chart.masterVolume == 55, "MASTER VOLUME 55");
  check(chart.outputChannel && *chart.outputChannel == 0, "CHANNEL 1");
  if (chart.warnings.size() < 2)
    dumpWarnings(chart, "BTT70S.PCH");
}

void testShadows()
{
  auto chart = loadChart("SHADOWS.PCH");
  checkEqual(chart.name, std::string("Shadows!"), "SHADOWS name");
  check(chart.programChangeNumber && *chart.programChangeNumber == 86, "SHADOWS PC 86");
  expectOn(chart, 4, true, "SHADOWS EQ on");
  expectOn(chart, 5, true, "SHADOWS NS on");
  expectOn(chart, 6, true, "SHADOWS short delay on");
  expectOn(chart, 8, true, "SHADOWS auto pan on");
  expectOn(chart, 10, true, "SHADOWS reverb on");
  expectOn(chart, 11, true, "SHADOWS lineout on");
  check(!chart.effectSlots[0].present, "SHADOWS no compressor");

  expectRaw(chart.effectSlots[4], 0x24, eqRaw("High", "Level", 8.0), "HI LEVEL +8.0 dB");
  expectRaw(chart.effectSlots[4], 0x28, eqRaw("Low Mid", "Freq", 346), "L.M. FREQ 346 Hz");
  expectRaw(chart.effectSlots[4], 0x29, eqRaw("Low Mid", "Q", 2.3), "L.M. Q 2.3");
  expectRaw(chart.effectSlots[4], 0x2A, eqRaw("Low Mid", "Level", -3.0), "L.M. LEVEL -3.0 dB");
  expectRaw(chart.effectSlots[4], 0x2C, eqRaw("Low", "Level", -6.0), "LO LEVEL -6.0 dB");
  expectRaw(chart.effectSlots[4], 0x2D, eqRaw("Out", "Level", 8.5), "OUT LEVEL +8.5 dB");

  expectRaw(chart.effectSlots[5], 0x2E, 100, "NS SENS 100");
  expectRaw(chart.effectSlots[5], 0x2F, 65, "NS RELEASE 65");
  expectRaw(chart.effectSlots[6], 0x31, 100, "D. TIME 100 msec");
  expectRaw(chart.effectSlots[6], 0x32, 36, "E. LEVEL 36");

  expectRaw(chart.effectSlots[8], 0x42, 80, "PAN RATE 80");
  expectRaw(chart.effectSlots[8], 0x43, 70, "PAN DEPTH 70");
  expectRaw(chart.effectSlots[8], 0x44, 1, "MODE trem");

  expectRaw(chart.effectSlots[10], 0x51, 35, "DECAY 4.0 sec");
  expectRaw(chart.effectSlots[10], 0x52, 8, "MODE SPRING 1");
  expectRaw(chart.effectSlots[10], 0x53, 200, "CUTOFF THRU");
  expectRaw(chart.effectSlots[10], 0x55, 0, "PRE DELAY 0 msec");
  expectRaw(chart.effectSlots[10], 0x56, 22, "E. LEVEL 22");

  expectRaw(chart.effectSlots[11], 0x57, 35, "PRESENCE 35");
  expectRaw(chart.effectSlots[11], 0x59, 70, "MIDDLE 70");
  check(chart.masterVolume && *chart.masterVolume == 100, "MASTER VOLUME 100");
  check(chart.warnings.empty(), "SHADOWS has no warnings");
  if (!chart.warnings.empty())
    dumpWarnings(chart, "SHADOWS.PCH");
}

void testQuirkSamples()
{
  auto edge = loadChart("EDGEECHO.PCH");
  check(edge.effectSlots[1].kind == EffectKind::Overdrive, "EDGEECHO A-2 overdrive");
  check(edge.isDistortion && !*edge.isDistortion, "EDGEECHO overdrive bit");
  expectRaw(edge.effectSlots[1], 0x16, specRaw(EffectKind::Overdrive, "Tone", -15), "TONE -15");
  expectRaw(edge.effectSlots[1], 0x18, 1, "TURBO ON");
  expectRaw(edge.effectSlots[7], 0x41, 2, "SPACE-D MODE 3");
  expectRaw(edge.effectSlots[9], 0x45, 400, "C. TAP 400 msec");
  expectRaw(edge.effectSlots[9], 0x4F, 192, "CUTOFF 7.26 kHz");
  expectRaw(edge.effectSlots[10], 0x51, 3, "DECAY 0.8 sec");

  auto space = loadChart("SPACEYL.PCH");
  check(space.effectSlots[7].kind == EffectKind::PitchShifter, "SPACEYL pitch shifter");
  expectRaw(space.effectSlots[7], 0x3B, 200, "BAL. E. 100 / BAL. D. 0");
  expectRaw(space.effectSlots[7], 0x3D, specRaw(EffectKind::PitchShifter, "Chromatic", 7),
            "CHROMATIC +7");
  expectRaw(space.effectSlots[7], 0x3E, specRaw(EffectKind::PitchShifter, "Fine", 0), "FINE 0");
  checkEqual(space.blockAOrder, (std::array<int, 6>{0, 4, 1, 2, 3, 5}), "SPACEYL seq A 152346");
  checkEqual(space.blockBOrder, (std::array<int, 6>{6, 8, 9, 7, 10, 11}), "SPACEYL seq B 134256");

  auto lux = loadChart("PICKLUX.PCH");
  expectRaw(lux.effectSlots[4], 0x23, eqRaw("High", "Freq", 8000), "8.0 kHz");
  expectRaw(lux.effectSlots[10], 0x52, 3, "MODE HALL 1");
  check(lux.effectSlots[7].kind == EffectKind::Chorus, "PICKLUX chorus");

  auto mrcs = loadChart("MRCS.PCH");
  checkEqual(mrcs.comments,
             std::string("My live sound, a bit much of everything if you aren't playing live"),
             "MRCS wrapped comments");
}

void testChartToPatchRoundTrip()
{
  int count = 0;
  for (const auto& entry : std::filesystem::directory_iterator(repoRoot() / "patches")) {
    if (entry.path().extension() != ".PCH")
      continue;
    ++count;
    const auto name = entry.path().filename().string();
    std::string error;
    const auto chart = parsePatchChartFile(entry.path(), error);
    check(error.empty(), name + " convert opened: " + error);
    const Patch patch = chartToPatch(chart);
    check(patch.isPresent(), name + " patch present");

    std::string expectedName = chart.name;
    if (expectedName.size() > 16)
      expectedName.resize(16);
    while (!expectedName.empty() && expectedName.back() == ' ')
      expectedName.pop_back();
    checkEqual(patch.name(), expectedName, name + " name field");

    checkEqual(patch.blockAOrder(), chart.blockAOrder, name + " block A order");
    checkEqual(patch.blockBOrder(), chart.blockBOrder, name + " block B order");
    checkEqual(patch.blockB2Mode(), chart.blockB2Mode.value_or(0), name + " B-2 mode");
    checkEqual(patch.isDistortion(), chart.isDistortion.value_or(true), name + " distortion bit");

    for (int id = 0; id < Patch::kEffectCount; ++id) {
      const auto& slot = chart.effectSlots[static_cast<std::size_t>(id)];
      checkEqual(patch.isEffectEnabled(id), slot.present,
                 name + " enable " + std::to_string(id));
      if (!slot.present)
        continue;
      for (const auto& field : slot.fields) {
        if (field.byteWidth >= 2)
          checkEqual(patch.wordAt(field.offset), field.raw,
                     name + " word @" + std::to_string(field.offset));
        else
          checkEqual(static_cast<int>(patch.byteAt(field.offset)), field.raw,
                     name + " byte @" + std::to_string(field.offset));
      }
    }

    if (chart.masterVolume)
      checkEqual(static_cast<int>(patch.byteAt(0x5B)), *chart.masterVolume, name + " master volume");
    if (chart.outputChannel)
      checkEqual(static_cast<int>(patch.byteAt(0x63)), *chart.outputChannel, name + " channel");
  }
  checkEqual(count, 13, "converted all 13 .PCH files");
}

void testChartToPatchAcoustic()
{
  const Patch patch = chartToPatch(loadChart("ACOUSTIC.PCH"));
  checkEqual(patch.name(), std::string("Acoustic"), "ACOUSTIC patch name");
  checkEqual(patch.playModeLcdLine2(), std::string("A-1***56B-*2**56"), "ACOUSTIC LCD line 2");
  check(patch.isEffectEnabled(0) && patch.isEffectEnabled(4) && patch.isEffectEnabled(5),
        "ACOUSTIC A-1/A-5/A-6 on");
  check(!patch.isEffectEnabled(1) && !patch.isEffectEnabled(2) && !patch.isEffectEnabled(3),
        "ACOUSTIC A-2/A-3/A-4 off");
  check(patch.blockB2Mode() == 3, "ACOUSTIC Space-D");
  checkEqual(static_cast<int>(patch.byteAt(0x11)), 80, "ACOUSTIC sustain 80");
  checkEqual(static_cast<int>(patch.byteAt(0x5B)), 75, "ACOUSTIC MASTER VOLUME 75");
  checkEqual(static_cast<int>(patch.byteAt(0x63)), 0, "ACOUSTIC CHANNEL 1");
  checkEqual(patch.wordAt(0x53), 200, "ACOUSTIC reverb CUTOFF THRU");
}

void testChartToPatchBtt70s()
{
  const Patch patch = chartToPatch(loadChart("BTT70S.PCH"));
  checkEqual(patch.blockAOrder(), (std::array<int, 6>{0, 2, 1, 4, 3, 5}), "BTT70S joint A");
  checkEqual(static_cast<int>(patch.byteAt(0x00)), 0, "BTT70S joint A0");
  checkEqual(static_cast<int>(patch.byteAt(0x01)), 2, "BTT70S joint A1");
  checkEqual(static_cast<int>(patch.byteAt(0x05)), 5, "BTT70S NS joint fixed");
  check(patch.isEffectEnabled(1) && patch.isEffectEnabled(2) && patch.isEffectEnabled(4),
        "BTT70S dist/filter/EQ on");
  check(!patch.isEffectEnabled(3), "BTT70S empty phaser stays off");
  check(!patch.isEffectEnabled(5), "BTT70S NS off — trust values, not ON/OFF row");
  checkEqual(static_cast<int>(patch.byteAt(0x5B)), 55, "BTT70S MASTER VOLUME 55");
}

void testChartToPatchEdlund2()
{
  const Patch patch = chartToPatch(loadChart("EDLUND2.PCH"));
  check(patch.isEffectEnabled(7), "EDLUND2 chorus on from printed values");
  check(!patch.isEffectEnabled(9), "EDLUND2 tap delay off — no section");
  checkEqual(patch.blockBOrder(), (std::array<int, 6>{6, 9, 7, 10, 8, 11}), "EDLUND2 seq B 142536");
}

} // namespace

int main()
{
  testAllPchParse();
  testAcoustic();
  testKnopfler();
  testBtt70s();
  testShadows();
  testQuirkSamples();
  testChartToPatchRoundTrip();
  testChartToPatchAcoustic();
  testChartToPatchBtt70s();
  testChartToPatchEdlund2();

  if (failures == 0) {
    std::cout << "All patch chart parser tests passed.\n";
    return 0;
  }
  std::cerr << failures << " patch chart parser test(s) failed.\n";
  return 1;
}

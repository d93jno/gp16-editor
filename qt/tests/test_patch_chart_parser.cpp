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

  check(chart.slots[0].present, "A-1 present");
  check(!chart.slots[1].present, "A-2 absent");
  check(chart.slots[4].present, "A-5 present");
  check(chart.slots[7].kind == EffectKind::SpaceD, "B-2 is Space-D");
  check(chart.blockB2Mode && *chart.blockB2Mode == 3, "B-2 mode Space-D");

  expectRaw(chart.slots[0], 0x0F, specRaw(EffectKind::Compressor, "Tone", 0), "TONE 0");
  expectRaw(chart.slots[0], 0x10, 20, "ATTACK 20");
  expectRaw(chart.slots[0], 0x11, 80, "SUSTAIN 80");
  expectRaw(chart.slots[0], 0x12, 40, "LEVEL 40");

  expectRaw(chart.slots[4], 0x23, eqRaw("High", "Freq", 8000), "HI FREQ 8.00 kHz");
  expectRaw(chart.slots[4], 0x24, eqRaw("High", "Level", 11.0), "HI LEVEL +11.0 dB");
  expectRaw(chart.slots[4], 0x25, eqRaw("High Mid", "Freq", 2980), "H.M.FREQ 2.98 kHz");
  expectRaw(chart.slots[4], 0x26, eqRaw("High Mid", "Q", 1.0), "H.M. Q 1.0");
  expectRaw(chart.slots[4], 0x27, eqRaw("High Mid", "Level", 0.5), "H.M. LEVEL +0.5 dB");
  expectRaw(chart.slots[4], 0x28, eqRaw("Low Mid", "Freq", 535), "L.M. FREQ 535 Hz");
  expectRaw(chart.slots[4], 0x29, eqRaw("Low Mid", "Q", 1.0), "L.M. Q 1.0");
  expectRaw(chart.slots[4], 0x2A, eqRaw("Low Mid", "Level", 3.5), "L.M. LEVEL +3.5 dB");
  expectRaw(chart.slots[4], 0x2B, eqRaw("Low", "Freq", 250), "LO FREQ 250 Hz");
  expectRaw(chart.slots[4], 0x2C, eqRaw("Low", "Level", 1.5), "LO LEVEL +1.5 dB");
  expectRaw(chart.slots[4], 0x2D, eqRaw("Out", "Level", 5.5), "OUT LEVEL +5.5 dB");

  expectRaw(chart.slots[5], 0x2E, 32, "NS SENS 32");
  expectRaw(chart.slots[5], 0x2F, 0, "NS RELEASE 0");
  expectRaw(chart.slots[5], 0x30, 100, "NS LEVEL 100");

  expectRaw(chart.slots[7], 0x41, 0, "SPACE-D MODE 1");

  expectRaw(chart.slots[10], 0x51, 5, "DECAY 1.0 sec");
  expectRaw(chart.slots[10], 0x52, 0, "MODE ROOM 1");
  expectRaw(chart.slots[10], 0x53, 200, "CUTOFF THRU");
  expectRaw(chart.slots[10], 0x55, 49, "PRE DELAY 49 msec");
  expectRaw(chart.slots[10], 0x56, 38, "E. LEVEL 38");

  expectRaw(chart.slots[11], 0x57, 10, "PRESENCE 10");
  expectRaw(chart.slots[11], 0x58, 50, "TREBLE 50");
  expectRaw(chart.slots[11], 0x59, 82, "MIDDLE 82");
  expectRaw(chart.slots[11], 0x5A, 78, "BASS 78");

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
  check(chart.slots[1].kind == EffectKind::Distortion, "A-2 Distortion kind");
  expectOn(chart, 0, true, "KNOPFLER A-1 on");
  expectOn(chart, 1, true, "KNOPFLER A-2 on");
  expectOn(chart, 11, true, "KNOPFLER B-6 on");
  check(!chart.slots[7].present, "KNOPFLER no B-2 section");

  expectRaw(chart.slots[1], 0x13, specRaw(EffectKind::Distortion, "Tone", 0), "DIST TONE 0");
  expectRaw(chart.slots[1], 0x14, 31, "DISTORTION 31");
  expectRaw(chart.slots[1], 0x15, 100, "DIST LEVEL 100");

  expectRaw(chart.slots[4], 0x23, eqRaw("High", "Freq", 8000), "KNOPFLER HI FREQ");
  expectRaw(chart.slots[4], 0x24, eqRaw("High", "Level", 0.0), "HI LEVEL 0.0 dB");
  expectRaw(chart.slots[4], 0x25, eqRaw("High Mid", "Freq", 1700), "H.M.FREQ 1.70 kHz");
  expectRaw(chart.slots[4], 0x27, eqRaw("High Mid", "Level", 7.5), "H.M. LEVEL 7.5 dB");
  expectRaw(chart.slots[4], 0x28, eqRaw("Low Mid", "Freq", 1000), "L.M. FREQ 1.00 kHz");
  expectRaw(chart.slots[4], 0x2B, eqRaw("Low", "Freq", 60), "LO FREQ 60 Hz");
  expectRaw(chart.slots[4], 0x2C, eqRaw("Low", "Level", 2.0), "LO LEVEL 2.0 dB");
  expectRaw(chart.slots[4], 0x2D, eqRaw("Out", "Level", 5.5), "OUT LEVEL +5.5 dB");

  expectRaw(chart.slots[5], 0x2E, 28, "NS SENS 28");
  expectRaw(chart.slots[5], 0x30, 52, "NS LEVEL 52");
  expectRaw(chart.slots[11], 0x57, 25, "PRESENCE 25");
  expectRaw(chart.slots[11], 0x59, 100, "MIDDLE 100");
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

  check(chart.slots[3].sawHeader, "A-4 header printed");
  check(!chart.slots[3].present, "A-4 has no values");
  check(!chart.slots[5].present, "A-6 section is missing");
  check(chart.slots[5].sawHeader == false, "A-6 has no header");

  check(warningContains(chart, "A-4"), "warning for empty A-4 section");
  check(warningContains(chart, "A-6"), "warning for NS on with no section");
  check(warningContains(chart, "expression pedal"), "warning that expression is not imported");

  expectRaw(chart.slots[1], 0x14, 41, "DISTORTION 41");
  expectRaw(chart.slots[2], 0x1A, 22, "PF SENS 22");
  expectRaw(chart.slots[2], 0x1B, 25, "CUTOFF FREQ 25");
  expectRaw(chart.slots[2], 0x1C, specRaw(EffectKind::PickingFilter, "Q", 4.5), "Q 4.5");
  expectRaw(chart.slots[2], 0x1D, 0, "UP/DOWN UP");

  expectRaw(chart.slots[4], 0x23, eqRaw("High", "Freq", 6060), "HI FREQ 6.06 kHz");
  expectRaw(chart.slots[4], 0x28, eqRaw("Low Mid", "Freq", 1000), "L.M. FREQ 1.00 kHz");
  expectRaw(chart.slots[4], 0x29, eqRaw("Low Mid", "Q", 1.9), "L.M. Q 1.9");
  expectRaw(chart.slots[4], 0x2C, eqRaw("Low", "Level", 7.0), "LO LEVEL +7.0 dB");
  expectRaw(chart.slots[4], 0x2D, eqRaw("Out", "Level", 6.0), "OUT LEVEL +6.0 dB");

  expectRaw(chart.slots[10], 0x51, 19, "DECAY 2.4 sec");
  expectRaw(chart.slots[10], 0x52, 9, "MODE SPRING 2");
  expectRaw(chart.slots[10], 0x53, 200, "CUTOFF THRU");
  expectRaw(chart.slots[10], 0x55, 25, "PRE DELAY 25 msec");
  expectRaw(chart.slots[10], 0x56, 18, "E. LEVEL 18");

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
  check(!chart.slots[0].present, "SHADOWS no compressor");

  expectRaw(chart.slots[4], 0x24, eqRaw("High", "Level", 8.0), "HI LEVEL +8.0 dB");
  expectRaw(chart.slots[4], 0x28, eqRaw("Low Mid", "Freq", 346), "L.M. FREQ 346 Hz");
  expectRaw(chart.slots[4], 0x29, eqRaw("Low Mid", "Q", 2.3), "L.M. Q 2.3");
  expectRaw(chart.slots[4], 0x2A, eqRaw("Low Mid", "Level", -3.0), "L.M. LEVEL -3.0 dB");
  expectRaw(chart.slots[4], 0x2C, eqRaw("Low", "Level", -6.0), "LO LEVEL -6.0 dB");
  expectRaw(chart.slots[4], 0x2D, eqRaw("Out", "Level", 8.5), "OUT LEVEL +8.5 dB");

  expectRaw(chart.slots[5], 0x2E, 100, "NS SENS 100");
  expectRaw(chart.slots[5], 0x2F, 65, "NS RELEASE 65");
  expectRaw(chart.slots[6], 0x31, 100, "D. TIME 100 msec");
  expectRaw(chart.slots[6], 0x32, 36, "E. LEVEL 36");

  expectRaw(chart.slots[8], 0x42, 80, "PAN RATE 80");
  expectRaw(chart.slots[8], 0x43, 70, "PAN DEPTH 70");
  expectRaw(chart.slots[8], 0x44, 1, "MODE trem");

  expectRaw(chart.slots[10], 0x51, 35, "DECAY 4.0 sec");
  expectRaw(chart.slots[10], 0x52, 8, "MODE SPRING 1");
  expectRaw(chart.slots[10], 0x53, 200, "CUTOFF THRU");
  expectRaw(chart.slots[10], 0x55, 0, "PRE DELAY 0 msec");
  expectRaw(chart.slots[10], 0x56, 22, "E. LEVEL 22");

  expectRaw(chart.slots[11], 0x57, 35, "PRESENCE 35");
  expectRaw(chart.slots[11], 0x59, 70, "MIDDLE 70");
  check(chart.masterVolume && *chart.masterVolume == 100, "MASTER VOLUME 100");
  check(chart.warnings.empty(), "SHADOWS has no warnings");
  if (!chart.warnings.empty())
    dumpWarnings(chart, "SHADOWS.PCH");
}

void testQuirkSamples()
{
  auto edge = loadChart("EDGEECHO.PCH");
  check(edge.slots[1].kind == EffectKind::Overdrive, "EDGEECHO A-2 overdrive");
  check(edge.isDistortion && !*edge.isDistortion, "EDGEECHO overdrive bit");
  expectRaw(edge.slots[1], 0x16, specRaw(EffectKind::Overdrive, "Tone", -15), "TONE -15");
  expectRaw(edge.slots[1], 0x18, 1, "TURBO ON");
  expectRaw(edge.slots[7], 0x41, 2, "SPACE-D MODE 3");
  expectRaw(edge.slots[9], 0x45, 400, "C. TAP 400 msec");
  expectRaw(edge.slots[9], 0x4F, 192, "CUTOFF 7.26 kHz");
  expectRaw(edge.slots[10], 0x51, 3, "DECAY 0.8 sec");

  auto space = loadChart("SPACEYL.PCH");
  check(space.slots[7].kind == EffectKind::PitchShifter, "SPACEYL pitch shifter");
  expectRaw(space.slots[7], 0x3B, 200, "BAL. E. 100 / BAL. D. 0");
  expectRaw(space.slots[7], 0x3D, specRaw(EffectKind::PitchShifter, "Chromatic", 7),
            "CHROMATIC +7");
  expectRaw(space.slots[7], 0x3E, specRaw(EffectKind::PitchShifter, "Fine", 0), "FINE 0");
  checkEqual(space.blockAOrder, (std::array<int, 6>{0, 4, 1, 2, 3, 5}), "SPACEYL seq A 152346");
  checkEqual(space.blockBOrder, (std::array<int, 6>{6, 8, 9, 7, 10, 11}), "SPACEYL seq B 134256");

  auto lux = loadChart("PICKLUX.PCH");
  expectRaw(lux.slots[4], 0x23, eqRaw("High", "Freq", 8000), "8.0 kHz");
  expectRaw(lux.slots[10], 0x52, 3, "MODE HALL 1");
  check(lux.slots[7].kind == EffectKind::Chorus, "PICKLUX chorus");

  auto mrcs = loadChart("MRCS.PCH");
  checkEqual(mrcs.comments,
             std::string("My live sound, a bit much of everything if you aren't playing live"),
             "MRCS wrapped comments");
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

  if (failures == 0) {
    std::cout << "All patch chart parser tests passed.\n";
    return 0;
  }
  std::cerr << failures << " patch chart parser test(s) failed.\n";
  return 1;
}

#include "Patch.h"
#include "PatchBank.h"

#include <cstdint>
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

std::filesystem::path repoRoot()
{
  return std::filesystem::path(GP16_TEST_REPO_ROOT);
}

void testDisplayId()
{
  checkEqual(Patch::displayIdFor(0), std::string("A11"), "index 0 -> A11");
  checkEqual(Patch::displayIdFor(7), std::string("A18"), "index 7 -> A18");
  checkEqual(Patch::displayIdFor(8), std::string("A21"), "index 8 -> A21");
  checkEqual(Patch::displayIdFor(63), std::string("A88"), "index 63 -> A88");
  checkEqual(Patch::displayIdFor(64), std::string("B11"), "index 64 -> B11");
  checkEqual(Patch::displayIdFor(127), std::string("B88"), "index 127 -> B88");

  checkEqual(Patch::groupLetterFor(0), 'A', "index 0 group A");
  checkEqual(Patch::bankDigitFor(0), 1, "index 0 bank 1");
  checkEqual(Patch::numberDigitFor(0), 1, "index 0 number 1");
  checkEqual(Patch::groupLetterFor(127), 'B', "index 127 group B");
  checkEqual(Patch::bankDigitFor(127), 8, "index 127 bank 8");
  checkEqual(Patch::numberDigitFor(127), 8, "index 127 number 8");
}

void testPlayModeLcd()
{
  std::vector<std::uint8_t> data(0x74, 0);
  for (int i = 0; i < 6; ++i)
    data[static_cast<std::size_t>(i)] = static_cast<std::uint8_t>(i);
  for (int i = 0; i < 6; ++i)
    data[static_cast<std::size_t>(6 + i)] = static_cast<std::uint8_t>(6 + i);
  data[0x0D] = 0b0001'0101;
  data[0x0E] = 0b0100'0001;
  const char* name = "Sparkling    AMP";
  for (int i = 0; i < 16; ++i)
    data[static_cast<std::size_t>(0x64 + i)] = static_cast<std::uint8_t>(name[i]);

  Patch p;
  p.parse(data, 0);
  checkEqual(p.playModeLcdLine1(), std::string(name), "LCD line 1 is the 16-char name field");
  checkEqual(p.playModeLcdLine2(), std::string("A-1*****B-12*4*6"),
             "LCD line 2 is A-AAAAAAB-BBBBBB (on=digit off=*)");

  Patch empty;
  checkEqual(empty.playModeLcdLine1(), std::string(16, ' '), "absent patch LCD line 1 is spaces");
  checkEqual(empty.playModeLcdLine2(), std::string(16, ' '), "absent patch LCD line 2 is spaces");
}

void testEffectEnableBits()
{
  // Synthetic patch: only the joint-data (0x00-0x0B) and on/off (0x0C-0x0E) bytes matter here.
  std::vector<std::uint8_t> data(0x0F, 0);
  data[0x0C] = 0x02;        // Block B-2 mode = Pitch Shifter
  data[0x0D] = 0b0001'0101; // a=0(distortion) x=0 b=1(B-6) c=0(B-5) d=1(B-4) e=0(B-3) f=1(B-2)
  data[0x0E] = 0b0100'0001; // g=1(B-1) h=0 i=0 j=0 k=0 l=0 m=1(A-1)

  Patch p;
  p.parse(data, 5);

  checkEqual(p.blockB2Mode(), 2, "blockB2Mode reads 0x0C & 0x03");
  check(p.isDistortion(), "0x0D bit6 clear selects the Distortion variant");

  check(p.isEffectEnabled(0), "Compressor (A-1) enabled via 0x0E bit0");
  check(!p.isEffectEnabled(1), "Distortion/Overdrive (A-2) disabled");
  check(!p.isEffectEnabled(2), "Picking Filter (A-3) disabled");
  check(p.isEffectEnabled(6), "Short Delay (B-1) enabled via 0x0E bit6");
  check(p.isEffectEnabled(7), "Block B-2 enabled via 0x0D bit0");
  check(!p.isEffectEnabled(8), "Auto Panpot (B-3) disabled");
  check(p.isEffectEnabled(9), "Tap Delay (B-4) enabled via 0x0D bit2");
  check(!p.isEffectEnabled(10), "Reverb (B-5) disabled");
  check(p.isEffectEnabled(11), "Lineout Filter (B-6) enabled via 0x0D bit4");

  p.setEffectEnabled(10, true);
  check(p.isEffectEnabled(10), "setEffectEnabled turns Reverb on");
  check((p.byteAt(0x0D) & 0x08) != 0, "setEffectEnabled writes the on/off bit back into data_");
}

void testWordAt()
{
  std::vector<std::uint8_t> data(0x11, 0);
  data[0x0F] = 0x03; // MSB
  data[0x10] = 0x7F; // LSB

  Patch p;
  p.parse(data, 0);
  checkEqual(p.wordAt(0x0F), (3 << 7) | 0x7F, "wordAt assembles (msb << 7) | lsb");

  p.setWordAt(0x0F, 1200);
  checkEqual(p.wordAt(0x0F), 1200, "setWordAt/wordAt round-trip 1200");
  checkEqual(static_cast<int>(p.byteAt(0x0F)), 1200 >> 7, "setWordAt MSB is value >> 7");
  checkEqual(static_cast<int>(p.byteAt(0x10)), 1200 & 0x7F, "setWordAt LSB is value & 0x7F");

  p.setByteAt(0x11, 77);
  checkEqual(static_cast<int>(p.byteAt(0x11)), 77, "setByteAt stores a 7-bit value");
}

void testPanelCapture()
{
  const auto path = repoRoot() / "captures" / "dump-20260730-153932.bin";
  PatchBank bank;
  std::string error;
  check(bank.loadFile(path, error), "panel capture loads: " + error);
  checkEqual(
      static_cast<int>(bank.shape()), static_cast<int>(PatchBank::IngestShape::PanelBulkDump),
      "panel capture detected as panel bulk dump");
  checkEqual(bank.presentCount(), 128, "panel capture yields 128 present patches");

  const auto& first = bank.patchAt(0);
  checkEqual(first.displayId(), std::string("A11"), "patch 0 display id");
  check(!first.name().empty(), "patch 0 has a non-empty name");
  checkEqual(static_cast<int>(first.playModeLcdLine1().size()), 16, "patch 0 LCD line 1 is 16 chars");
  checkEqual(static_cast<int>(first.playModeLcdLine2().size()), 16, "patch 0 LCD line 2 is 16 chars");
  check(first.playModeLcdLine1().find(first.name()) == 0,
        "trimmed name is the prefix of LCD line 1");
  for (char ch : first.playModeLcdLine2()) {
    check((ch >= '1' && ch <= '6') || ch == '*' || ch == ' ' || ch == 'A' || ch == 'B'
              || ch == '-',
          "LCD line 2 only uses digits, *, A-/B- and spaces");
  }

  const auto& last = bank.patchAt(127);
  checkEqual(last.displayId(), std::string("B88"), "patch 127 display id");
  check(!last.name().empty(), "patch 127 has a non-empty name");

  for (int i = 0; i < 128; ++i)
    check(bank.patchAt(i).isPresent(), "patch " + std::to_string(i) + " present in panel dump");
}

void testRq1CaptureAgreesWithPanelForGroupA()
{
  PatchBank panelBank;
  PatchBank rq1Bank;
  std::string error;
  check(
      panelBank.loadFile(repoRoot() / "captures" / "dump-20260730-153932.bin", error),
      "panel capture loads (cross-check): " + error);
  check(
      rq1Bank.loadFile(repoRoot() / "captures" / "midi-in.20260913-082751.bin", error),
      "RQ1 capture loads: " + error);

  checkEqual(
      static_cast<int>(rq1Bank.shape()), static_cast<int>(PatchBank::IngestShape::Rq1BulkDump),
      "RQ1 capture detected as RQ1 bulk dump");

  // Group A (indices 0-63) is fully populated in this capture; the two ingest
  // shapes read the same underlying patches and must agree byte-for-byte on
  // everything the parser derives from them.
  for (int i = 0; i < 64; ++i) {
    const auto& panelPatch = panelBank.patchAt(i);
    const auto& rq1Patch = rq1Bank.patchAt(i);
    const auto tag = std::to_string(i);
    check(rq1Patch.isPresent(), "RQ1 patch " + tag + " present");
    checkEqual(rq1Patch.name(), panelPatch.name(), "name agrees for index " + tag);
    checkEqual(rq1Patch.blockAOrder(), panelPatch.blockAOrder(), "block A order agrees for index " + tag);
    checkEqual(rq1Patch.blockBOrder(), panelPatch.blockBOrder(), "block B order agrees for index " + tag);
    checkEqual(rq1Patch.blockB2Mode(), panelPatch.blockB2Mode(), "block B2 mode agrees for index " + tag);
    checkEqual(rq1Patch.isDistortion(), panelPatch.isDistortion(), "A-2 variant agrees for index " + tag);
    for (int id = 0; id < Patch::kEffectCount; ++id) {
      checkEqual(
          rq1Patch.isEffectEnabled(id), panelPatch.isEffectEnabled(id),
          "on/off agrees for index " + tag + " effect " + std::to_string(id));
    }
  }
}

void testMalformedInput()
{
  std::string error;

  PatchBank emptyBank;
  const std::vector<std::uint8_t> empty;
  check(!emptyBank.ingestBytes(empty, error), "empty input is rejected");
  check(!error.empty(), "empty input yields an error message");

  PatchBank junkBank;
  const std::vector<std::uint8_t> junk{0x00, 0x01, 0x02};
  check(!junkBank.ingestBytes(junk, error), "non-SysEx junk is rejected");
}

} // namespace

int main()
{
  testDisplayId();
  testPlayModeLcd();
  testEffectEnableBits();
  testWordAt();
  testPanelCapture();
  testRq1CaptureAgreesWithPanelForGroupA();
  testMalformedInput();

  if (failures == 0) {
    std::cout << "All tests passed.\n";
    return 0;
  }
  std::cerr << failures << " test(s) failed.\n";
  return 1;
}

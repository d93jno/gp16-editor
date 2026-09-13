#include "Patch.h"
#include "PatchBank.h"
#include "PatchDisplayWidget.h"

#include <QApplication>
#include <QByteArray>

#include <filesystem>
#include <iostream>
#include <string>

namespace {

int failures = 0;

void check(bool condition, const std::string& what)
{
  if (!condition) {
    std::cerr << "FAIL: " << what << "\n";
    ++failures;
  }
}

std::filesystem::path repoRoot()
{
  return std::filesystem::path(GP16_TEST_REPO_ROOT);
}

} // namespace

int main(int argc, char* argv[])
{
  qputenv("QT_QPA_PLATFORM", QByteArrayLiteral("offscreen"));
  QApplication app(argc, argv);

  PatchDisplayWidget widget;
  widget.show();

  check(widget.groupLetter() == '\0', "empty display has no group");
  check(widget.bankDigit() == 0, "empty display has no bank digit");
  check(widget.numberDigit() == 0, "empty display has no number digit");
  check(widget.lcdLine1() == QString(16, QLatin1Char(' ')), "empty LCD line 1 is spaces");
  check(widget.lcdLine2() == QString(16, QLatin1Char(' ')), "empty LCD line 2 is spaces");

  PatchBank bank;
  std::string error;
  check(bank.loadFile(repoRoot() / "captures" / "dump-20260730-153932.bin", error),
        "load dump-20260730-153932.bin: " + error);

  auto& first = bank.patchAt(0);
  widget.setPatch(&first);
  check(widget.groupLetter() == 'A', "patch 0 group A");
  check(widget.bankDigit() == 1, "patch 0 bank 1");
  check(widget.numberDigit() == 1, "patch 0 number 1");
  check(widget.lcdLine1() == QString::fromStdString(first.playModeLcdLine1()),
        "widget LCD line 1 matches Patch");
  check(widget.lcdLine2() == QString::fromStdString(first.playModeLcdLine2()),
        "widget LCD line 2 matches Patch");

  auto& last = bank.patchAt(127);
  widget.setPatch(&last);
  check(widget.groupLetter() == 'B', "patch 127 group B");
  check(widget.bankDigit() == 8, "patch 127 bank 8");
  check(widget.numberDigit() == 8, "patch 127 number 8");

  int firstIndex = -1;
  int differingIndex = -1;
  for (int i = 0; i < PatchBank::kPatchCount && differingIndex < 0; ++i) {
    if (!bank.hasPatch(i))
      continue;
    if (firstIndex < 0) {
      firstIndex = i;
      continue;
    }
    if (bank.patchAt(i).playModeLcdLine2() != bank.patchAt(firstIndex).playModeLcdLine2())
      differingIndex = i;
  }
  check(firstIndex >= 0, "capture has a present patch");
  check(differingIndex >= 0, "capture contains two patches with different LCD line 2");

  widget.setPatch(nullptr);
  check(widget.groupLetter() == '\0', "clearing the patch blanks the group");
  check(widget.bankDigit() == 0, "clearing the patch blanks the bank digit");

  if (failures == 0) {
    std::cout << "All patch display tests passed.\n";
    return 0;
  }
  std::cerr << failures << " patch display test(s) failed.\n";
  return 1;
}

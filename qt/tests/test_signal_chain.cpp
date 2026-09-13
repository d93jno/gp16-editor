#include "Patch.h"
#include "PatchBank.h"
#include "SignalChainWidget.h"

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

void checkPatchAgainstWidget(SignalChainWidget& widget, Patch& patch, const std::string& label)
{
  widget.setPatch(&patch);
  for (int i = 0; i < 6; ++i) {
    const int identityA = patch.blockAOrder()[static_cast<std::size_t>(i)];
    check(widget.chipIdentity(i, false) == identityA,
          label + " block A position " + std::to_string(i) + " identity");
    const auto expectedNameA = QString::fromStdString(
        Patch::effectName(identityA, patch.blockB2Mode(), patch.isDistortion()));
    check(widget.chipText(i, false) == expectedNameA,
          label + " block A position " + std::to_string(i) + " label");
    check(widget.chipChecked(i, false) == patch.isEffectEnabled(identityA),
          label + " block A position " + std::to_string(i) + " checked state");

    const int identityB = patch.blockBOrder()[static_cast<std::size_t>(i)];
    check(widget.chipIdentity(i, true) == identityB,
          label + " block B position " + std::to_string(i) + " identity");
    const auto expectedNameB = QString::fromStdString(
        Patch::effectName(identityB, patch.blockB2Mode(), patch.isDistortion()));
    check(widget.chipText(i, true) == expectedNameB,
          label + " block B position " + std::to_string(i) + " label");
    check(widget.chipChecked(i, true) == patch.isEffectEnabled(identityB),
          label + " block B position " + std::to_string(i) + " checked state");
  }
}

} // namespace

int main(int argc, char* argv[])
{
  qputenv("QT_QPA_PLATFORM", QByteArrayLiteral("offscreen"));
  QApplication app(argc, argv);

  SignalChainWidget widget;
  widget.show();

  for (int i = 0; i < 6; ++i) {
    check(widget.chipIdentity(i, false) == i, "default block A identity order");
    check(!widget.chipChecked(i, false), "default block A chip unchecked");
    check(widget.chipIdentity(i, true) == 6 + i, "default block B identity order");
    check(!widget.chipChecked(i, true), "default block B chip unchecked");
  }

  PatchBank bank;
  std::string error;
  check(bank.loadFile(repoRoot() / "captures" / "dump-20260730-153932.bin", error),
        "load dump-20260730-153932.bin: " + error);

  int firstIndex = -1;
  int differingIndex = -1;
  for (int i = 0; i < PatchBank::kPatchCount && differingIndex < 0; ++i) {
    if (!bank.hasPatch(i))
      continue;
    if (firstIndex < 0) {
      firstIndex = i;
      continue;
    }
    if (bank.patchAt(i).blockAOrder() != bank.patchAt(firstIndex).blockAOrder()
        || bank.patchAt(i).blockBOrder() != bank.patchAt(firstIndex).blockBOrder()
        || bank.patchAt(i).blockB2Mode() != bank.patchAt(firstIndex).blockB2Mode())
      differingIndex = i;
  }

  check(firstIndex >= 0, "capture has at least one present patch");
  if (firstIndex >= 0)
    checkPatchAgainstWidget(widget, bank.patchAt(firstIndex), "patch " + std::to_string(firstIndex));

  check(differingIndex >= 0, "capture contains two patches with differing joint data");
  if (differingIndex >= 0)
    checkPatchAgainstWidget(widget, bank.patchAt(differingIndex), "patch " + std::to_string(differingIndex));

  if (failures == 0) {
    std::cout << "All signal chain tests passed.\n";
    return 0;
  }
  std::cerr << failures << " signal chain test(s) failed.\n";
  return 1;
}

#include "PatchBank.h"
#include "PatchListPanel.h"

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

void testCapture(PatchListPanel& panel, const std::filesystem::path& path)
{
  PatchBank bank;
  std::string error;
  check(bank.loadFile(path, error), "load " + path.string() + ": " + error);

  panel.rebuild(bank);
  panel.clearFilter();
  check(panel.visibleCount() == 128, path.filename().string() + " shows 128 rows");

  const auto first = panel.rowText(0);
  check(first.startsWith(QStringLiteral("A11  ")),
        path.filename().string() + " row 0 starts A11");

  if (bank.patchAt(0).isPresent() && !bank.patchAt(0).name().empty()) {
    const auto name = QString::fromStdString(bank.patchAt(0).name());
    check(first.contains(name), path.filename().string() + " row 0 contains patch name");
    panel.setFilter(name);
    check(panel.visibleCount() >= 1, path.filename().string() + " search by name keeps a match");
  }

  panel.setFilter(QStringLiteral("A11"));
  check(panel.visibleCount() >= 1, path.filename().string() + " search A11 keeps a match");

  panel.setFilter(QStringLiteral("ZZZZ-NO-SUCH-PATCH"));
  check(panel.visibleCount() == 0, path.filename().string() + " unmatched search hides every row");

  panel.clearFilter();
  check(panel.visibleCount() == 128, path.filename().string() + " clearing search restores 128 rows");

  const auto last = panel.rowText(127);
  check(last.startsWith(QStringLiteral("B88  ")),
        path.filename().string() + " row 127 starts B88");
  if (bank.patchAt(127).isPresent() && !bank.patchAt(127).name().empty()) {
    check(last.contains(QString::fromStdString(bank.patchAt(127).name())),
          path.filename().string() + " row 127 contains patch name");
  }
}

} // namespace

int main(int argc, char* argv[])
{
  qputenv("QT_QPA_PLATFORM", QByteArrayLiteral("offscreen"));
  QApplication app(argc, argv);

  PatchListPanel panel;
  panel.show();

  check(panel.rowText(0) == QStringLiteral("A11  —"), "placeholder row 0 is A11  —");
  check(panel.rowText(127) == QStringLiteral("B88  —"), "placeholder row 127 is B88  —");
  check(panel.visibleCount() == 128, "all 128 placeholder rows visible");

  testCapture(panel, repoRoot() / "captures" / "dump-20260730-153932.bin");
  testCapture(panel, repoRoot() / "device-dump.bin");
  testCapture(panel, repoRoot() / "midi-in.20260913-082751.bin");
  testCapture(panel, repoRoot() / "midi-in.20260913-091336.bin");

  if (failures == 0) {
    std::cout << "All librarian tests passed.\n";
    return 0;
  }
  std::cerr << failures << " librarian test(s) failed.\n";
  return 1;
}

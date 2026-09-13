#include "EffectEditor.h"
#include "MainWindow.h"
#include "SignalChainWidget.h"

#include <QApplication>
#include <QByteArray>
#include <QToolButton>

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

QToolButton* firstChainChip(MainWindow& window)
{
  auto* chain = window.findChild<SignalChainWidget*>();
  if (!chain)
    return nullptr;
  const auto chips = chain->findChildren<QToolButton*>();
  return chips.isEmpty() ? nullptr : chips.front();
}

} // namespace

int main(int argc, char* argv[])
{
  qputenv("QT_QPA_PLATFORM", QByteArrayLiteral("offscreen"));
  QApplication app(argc, argv);

  MainWindow window;
  window.show();

  auto* editor = window.findChild<EffectEditor*>();
  check(editor != nullptr, "MainWindow hosts an EffectEditor");
  auto* chip = firstChainChip(window);
  check(chip != nullptr, "signal chain has a chip to click");
  if (!editor || !chip)
    return 1;

  check(editor->identity() == -1, "startup has no selected slot");
  check(editor->paramCount() == 0, "startup editor is empty");

  chip->click();
  check(editor->identity() == -1, "chip click with no librarian row does not select a slot");
  check(editor->paramCount() == 0, "chip click with no librarian row does not bind the editor");
  check(editor->titleText().isEmpty(), "chip click with no librarian row leaves the title empty");

  check(window.openDumpFile(QString::fromStdString(
            (repoRoot() / "captures" / "dump-20260730-153932.bin").string())),
        "open dump-20260730-153932.bin");

  chip->click();
  check(editor->identity() == 0, "chip click after a row is selected binds compressor");
  check(editor->paramCount() > 0, "chip click after a row is selected shows parameters");

  if (failures == 0) {
    std::cout << "All main window tests passed.\n";
    return 0;
  }
  std::cerr << failures << " main window test(s) failed.\n";
  return 1;
}

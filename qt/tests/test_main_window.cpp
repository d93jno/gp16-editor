#include "EffectEditor.h"
#include "EffectSpecs.h"
#include "MainWindow.h"
#include "SignalChainWidget.h"

#include <QApplication>
#include <QByteArray>
#include <QCoreApplication>
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

  // Phase 6: editing must stay functional with no MIDI output connected — the
  // coalescing send in MainWindow::onParameterEdited is skipped, not fatal.
  const auto& spec = specFor(editor->currentKind());
  check(!spec.params.empty(), "compressor page has a parameter to edit");
  if (!spec.params.empty()) {
    const auto& param = spec.params[0];
    const int before = editor->paramRawValue(0);
    const int changed = before == param.min ? param.max : param.min;
    editor->setParamRawValue(0, changed);
    check(editor->paramRawValue(0) == changed,
          "editing a parameter with no MIDI output open still updates the local model");
    QCoreApplication::processEvents();
    check(editor->paramRawValue(0) == changed,
          "the edit survives a coalescing tick while offline");
  }

  if (failures == 0) {
    std::cout << "All main window tests passed.\n";
    return 0;
  }
  std::cerr << failures << " main window test(s) failed.\n";
  return 1;
}

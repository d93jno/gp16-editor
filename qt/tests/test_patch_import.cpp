#include "EffectEditor.h"
#include "MainWindow.h"
#include "Patch.h"
#include "PatchBank.h"
#include "PatchChartParser.h"
#include "PatchDisplayWidget.h"
#include "PatchImportDialog.h"
#include "PatchListPanel.h"
#include "SignalChainWidget.h"

#include <QAction>
#include <QApplication>
#include <QList>
#include <QPushButton>
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

QString pchPath(const char* filename)
{
  return QString::fromStdString((repoRoot() / "patches" / filename).string());
}

void testDialogAcoustic()
{
  PatchBank bank;
  const auto chart = loadChart("ACOUSTIC.PCH");
  PatchImportDialog dialog(chart, bank, 0);
  checkEqual(dialog.destinationIndex(), 0, "default destination is A11");
  check(dialog.patchName().contains(QStringLiteral("Acoustic")), "preview shows patch name");
  check(dialog.authorText().contains(QStringLiteral("Jonas Nordin")), "preview shows author");
  check(dialog.commentsText().contains(QStringLiteral("steel-stringed")),
        "preview shows comments");
  checkEqual(dialog.warningCount(), 0, "ACOUSTIC has no warnings in the dialog");
  const auto effects = dialog.effectsSummary();
  check(effects.contains(QStringLiteral("Compressor")), "preview lists compressor");
  check(effects.contains(QStringLiteral("A-1")) && effects.contains(QStringLiteral("ON")),
        "A-1 is ON");
  check(effects.contains(QStringLiteral("Master Volume")) && effects.contains(QStringLiteral("75")),
        "preview shows MASTER VOLUME 75");
  check(effects.contains(QStringLiteral("Channel")) && effects.contains(QStringLiteral("1")),
        "preview shows CHANNEL 1");

  dialog.setDestinationIndex(72);
  checkEqual(dialog.destinationIndex(), 72, "destination picker accepts B21");

  auto* importBtn = dialog.findChild<QPushButton*>(QStringLiteral("importButton"));
  check(importBtn != nullptr, "Import button is present");
}

void testDialogBtt70sWarnings()
{
  PatchBank bank;
  const auto chart = loadChart("BTT70S.PCH");
  PatchImportDialog dialog(chart, bank, 64);
  checkEqual(dialog.destinationIndex(), 64, "default destination is the requested slot");
  check(dialog.warningCount() >= 2, "BTT70S warnings are listed in the dialog");
  bool sawNs = false;
  bool sawPhaser = false;
  bool sawExpr = false;
  for (int i = 0; i < dialog.warningCount(); ++i) {
    const auto w = dialog.warningAt(i);
    if (w.contains(QStringLiteral("A-6")))
      sawNs = true;
    if (w.contains(QStringLiteral("A-4")))
      sawPhaser = true;
    if (w.contains(QStringLiteral("expression"), Qt::CaseInsensitive))
      sawExpr = true;
  }
  check(sawNs, "dialog surfaces NS on/off disagreement");
  check(sawPhaser, "dialog surfaces empty A-4 header");
  check(sawExpr, "dialog surfaces skipped expression pedal");
}

void testImportIntoLibrarian()
{
  MainWindow window;
  window.show();

  auto* importAction = window.findChild<QAction*>(QStringLiteral("importPatchAction"));
  check(importAction != nullptr && importAction->isEnabled(),
        "Import Patch is enabled with no MIDI device");

  check(window.importPatchFile(pchPath("ACOUSTIC.PCH"), 72), "import ACOUSTIC into B21");

  auto* list = window.findChild<PatchListPanel*>();
  check(list != nullptr, "MainWindow hosts a librarian");
  if (list) {
    checkEqual(list->currentPatchIndex(), 72, "librarian selects B21 after import");
    const auto row = list->rowText(72);
    check(row.startsWith(QStringLiteral("B21")), "row 72 is labelled B21");
    check(row.contains(QStringLiteral("Acoustic")), "B21 row shows Acoustic");
    check(list->rowText(0) == QStringLiteral("A11  —"), "other slots stay empty");
  }

  auto* display = window.findChild<PatchDisplayWidget*>();
  check(display != nullptr, "front panel is present");
  if (display) {
    check(display->groupLetter() == 'B', "front panel group B");
    check(display->bankDigit() == 2, "front panel bank 2");
    check(display->numberDigit() == 1, "front panel number 1");
    check(display->lcdLine1().startsWith(QStringLiteral("Acoustic")),
          "front panel LCD name is Acoustic");
    checkEqual(display->lcdLine2().toStdString(), std::string("A-1***56B-*2**56"),
               "front panel LCD line 2 matches the chart");
  }

  auto* chain = window.findChild<SignalChainWidget*>();
  check(chain != nullptr, "signal chain is present");
  if (chain) {
    check(chain->chipChecked(0, false), "compressor chip is on");
    check(!chain->chipChecked(1, false), "distortion chip is off");
    check(chain->chipText(1, true).contains(QStringLiteral("Space")), "B-2 chip is Space-D");
  }

  auto* editor = window.findChild<EffectEditor*>();
  auto* chainWidget = window.findChild<SignalChainWidget*>();
  const auto chips = chainWidget ? chainWidget->findChildren<QToolButton*>() : QList<QToolButton*>{};
  check(editor != nullptr && !chips.isEmpty(), "editor and chain chips exist");
  if (editor && !chips.isEmpty()) {
    chips.front()->click();
    checkEqual(editor->identity(), 0, "compressor page after import");
    bool sawSustain = false;
    for (int i = 0; i < editor->paramCount(); ++i) {
      if (editor->paramOffset(i) == 0x11) {
        checkEqual(editor->paramRawValue(i), 80, "compressor sustain 80");
        sawSustain = true;
      }
    }
    check(sawSustain, "compressor sustain is on the editor page");
  }
}

void testImportOverDumpAndAllSamples()
{
  MainWindow window;
  window.show();
  check(window.openDumpFile(QString::fromStdString(
            (repoRoot() / "captures" / "dump-20260730-153932.bin").string())),
        "open a dump before importing over a slot");

  auto* list = window.findChild<PatchListPanel*>();
  check(list != nullptr, "librarian exists");
  const auto before = list ? list->rowText(0) : QString();
  check(!before.contains(QStringLiteral("Acoustic")), "dump slot 0 is not Acoustic yet");

  check(window.importPatchFile(pchPath("KNOPFLER.PCH"), 0), "import KNOPFLER over A11");
  if (list) {
    check(list->rowText(0).contains(QStringLiteral("Knopfler")), "A11 is now Knopfler sound");
    checkEqual(list->currentPatchIndex(), 0, "selection stays/moves to A11");
  }

  auto* display = window.findChild<PatchDisplayWidget*>();
  if (display) {
    check(display->lcdLine1().contains(QStringLiteral("Knopfler")),
          "front panel shows Knopfler after overwrite");
  }

  int imported = 0;
  for (const auto& entry : std::filesystem::directory_iterator(repoRoot() / "patches")) {
    if (entry.path().extension() != ".PCH")
      continue;
    const int dest = imported % PatchBank::kPatchCount;
    const auto path = QString::fromStdString(entry.path().string());
    check(window.importPatchFile(path, dest), "import " + entry.path().filename().string());
    ++imported;
  }
  checkEqual(imported, 13, "all 13 sample charts import with no device");
}

} // namespace

int main(int argc, char* argv[])
{
  qputenv("QT_QPA_PLATFORM", QByteArrayLiteral("offscreen"));
  QApplication app(argc, argv);

  testDialogAcoustic();
  testDialogBtt70sWarnings();
  testImportIntoLibrarian();
  testImportOverDumpAndAllSamples();

  if (failures == 0) {
    std::cout << "All patch import tests passed.\n";
    return 0;
  }
  std::cerr << failures << " patch import test(s) failed.\n";
  return 1;
}

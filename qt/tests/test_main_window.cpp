#include "EffectEditor.h"
#include "EffectSpecs.h"
#include "MainWindow.h"
#include "MidiService.h"
#include "PatchDisplayWidget.h"
#include "RolandSysex.h"
#include "SignalChainWidget.h"

#include <QAction>
#include <QApplication>
#include <QByteArray>
#include <QCoreApplication>
#include <QElapsedTimer>
#include <QEventLoop>
#include <QLabel>
#include <QStackedWidget>
#include <QToolButton>

#include <cstdint>
#include <filesystem>
#include <iostream>
#include <span>
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

roland::ParsedDt1 parseSent(const QByteArray& bytes)
{
  return roland::parseDt1(std::span<const std::uint8_t>(
      reinterpret_cast<const std::uint8_t*>(bytes.constData()),
      static_cast<std::size_t>(bytes.size())));
}

std::vector<roland::ParsedDt1> writesTo(const std::vector<QByteArray>& sent, int offset)
{
  std::vector<roland::ParsedDt1> hits;
  for (const auto& bytes : sent) {
    auto parsed = parseSent(bytes);
    if (parsed.valid && parsed.address.size() == 3 && parsed.address[0] == 0x00 &&
        parsed.address[1] == 0x00 && parsed.address[2] == offset)
      hits.push_back(std::move(parsed));
  }
  return hits;
}

// Spins the event loop (real wall-clock time, not a blind sleep) until either
// the predicate is true or timeoutMs elapses, so tests that depend on the
// live-edit coalescing timer (MainWindow.cpp, ~40 ms) wait on the actual
// QTimer::timeout rather than assuming a fixed delay is enough.
template <typename Predicate>
bool waitUntil(Predicate predicate, int timeoutMs)
{
  QElapsedTimer elapsed;
  elapsed.start();
  while (!predicate()) {
    if (elapsed.elapsed() >= timeoutMs)
      return predicate();
    QCoreApplication::processEvents(QEventLoop::AllEvents, 5);
  }
  return true;
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

  auto* display = window.findChild<PatchDisplayWidget*>();
  auto* stack = window.findChild<QStackedWidget*>(QStringLiteral("headerStack"));
  auto* nameHeader = window.findChild<QWidget*>(QStringLiteral("nameHeader"));
  auto* frontPanelAction =
      window.findChild<QAction*>(QStringLiteral("frontPanelDisplayAction"));
  check(display != nullptr, "MainWindow hosts a PatchDisplayWidget");
  check(stack != nullptr, "MainWindow hosts a header stack");
  check(nameHeader != nullptr, "MainWindow hosts the name header");
  check(frontPanelAction != nullptr, "View menu has Front panel display");
  check(stack && display && stack->currentWidget() == display,
        "front panel is the default view");
  check(frontPanelAction && frontPanelAction->isChecked(),
        "Front panel display is checked by default");
  auto* importAction = window.findChild<QAction*>(QStringLiteral("importPatchAction"));
  check(importAction != nullptr, "toolbar has Import Patch");
  check(importAction && importAction->isEnabled(), "Import Patch is enabled with no device");
  if (display) {
    check(display->groupLetter() == 'A', "opened dump selects group A");
    check(display->bankDigit() == 1, "opened dump selects bank 1");
    check(display->numberDigit() == 1, "opened dump selects number 1");
    check(display->lcdLine1().size() == 16, "LCD line 1 is 16 characters");
    check(display->lcdLine2().size() == 16, "LCD line 2 is 16 characters");
  }
  if (frontPanelAction && stack && display && nameHeader) {
    frontPanelAction->setChecked(false);
    check(stack->currentWidget() == nameHeader, "unchecking Front panel display restores the name header");
    frontPanelAction->setChecked(true);
    check(stack->currentWidget() == display, "checking Front panel display shows the cluster");
  }

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

  // Phase 6: coalescing/gating with a simulated output port. No real MIDI
  // backend is used (setTestOutputOpen), so this runs without hardware; the
  // actual wire bytes are captured via MidiService::sysExSent.
  window.midiService()->setTestOutputOpen(true);
  std::vector<QByteArray> sent;
  QObject::connect(window.midiService(), &MidiService::sysExSent,
                    [&sent](const QByteArray& bytes) { sent.push_back(bytes); });

  // Reverb (identity 10, fixed — not a variant slot) has a byteWidth==2
  // Cutoff parameter at 0x53/0x54.
  editor->setIdentity(10);
  check(editor->currentKind() == EffectKind::Reverb, "setIdentity(10) selects Reverb");
  const auto& reverbSpec = specFor(editor->currentKind());
  int cutoffIndex = -1;
  for (int i = 0; i < editor->paramCount(); ++i) {
    if (editor->paramOffset(i) == 0x53) {
      cutoffIndex = i;
      break;
    }
  }
  check(cutoffIndex >= 0, "Reverb page has a Cutoff parameter at offset 0x53");
  if (cutoffIndex >= 0) {
    check(reverbSpec.params[cutoffIndex].byteWidth == 2,
          "Reverb Cutoff is a two-byte (MSB/LSB) parameter");

    sent.clear();
    editor->setParamRawValue(cutoffIndex, 40);
    editor->setParamRawValue(cutoffIndex, 150); // same offset, before the tick — must collapse

    const bool flushed =
        waitUntil([&] { return !writesTo(sent, 0x53).empty(); }, 500);
    check(flushed, "the ~40 ms coalescing tick actually fires and sends the queued edit");

    const auto cutoffWrites = writesTo(sent, 0x53);
    check(cutoffWrites.size() == 1,
          "two edits to the same offset collapse into a single coalesced DT1");
    if (cutoffWrites.size() == 1) {
      check(cutoffWrites.front().data.size() == 2,
            "a byteWidth==2 send is one two-byte DT1, not two single-byte writes");
      if (cutoffWrites.front().data.size() == 2) {
        const int decoded = (cutoffWrites.front().data[0] << 7) | cutoffWrites.front().data[1];
        check(decoded == 150, "the coalesced write carries the last value, not the first");
      }
    }

    sent.clear();
    const bool scrSent = waitUntil([&] { return !writesTo(sent, 0x75).empty(); }, 500);
    check(scrSent, "a settled edit burst sends SOUND CHANGE REQUEST (00 00 75)");
    check(writesTo(sent, 0x75).size() <= 1,
          "SOUND CHANGE REQUEST is sent at most once per settled burst");
  }

  if (failures == 0) {
    std::cout << "All main window tests passed.\n";
    return 0;
  }
  std::cerr << failures << " main window test(s) failed.\n";
  return 1;
}

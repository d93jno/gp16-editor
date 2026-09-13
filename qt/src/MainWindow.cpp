#include "MainWindow.h"

#include "EffectEditor.h"
#include "MidiService.h"
#include "Patch.h"
#include "PatchListPanel.h"
#include "RolandSysex.h"
#include "SignalChainWidget.h"

#include <QAction>
#include <QComboBox>
#include <QDockWidget>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QLabel>
#include <QMenu>
#include <QMenuBar>
#include <QPlainTextEdit>
#include <QSignalBlocker>
#include <QSpinBox>
#include <QSplitter>
#include <QStatusBar>
#include <QTimer>
#include <QToolBar>
#include <QVBoxLayout>
#include <QWidget>

#include <span>

namespace {

QWidget* labeledWidget(const QString& label, QWidget* field, QWidget* parent)
{
  auto* wrap = new QWidget(parent);
  auto* layout = new QHBoxLayout(wrap);
  layout->setContentsMargins(4, 0, 8, 0);
  layout->setSpacing(4);
  auto* text = new QLabel(label, wrap);
  layout->addWidget(text);
  layout->addWidget(field);
  return wrap;
}

std::span<const std::uint8_t> asBytes(const QByteArray& data)
{
  return {reinterpret_cast<const std::uint8_t*>(data.constData()),
          static_cast<std::size_t>(data.size())};
}

} // namespace

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , midi_(new MidiService(this))
    , dumpTimeout_(new QTimer(this))
{
  setWindowTitle(QStringLiteral("GP-16 Editor"));
  resize(1100, 720);

  dumpTimeout_->setSingleShot(true);
  dumpTimeout_->setInterval(8000);
  connect(dumpTimeout_, &QTimer::timeout, this, &MainWindow::onDumpTimeout);

  auto* toolbar = addToolBar(QStringLiteral("MIDI"));
  toolbar->setMovable(false);
  toolbar->setFloatable(false);
  toolbar->setObjectName(QStringLiteral("midiToolbar"));

  inputCombo_ = new QComboBox(this);
  outputCombo_ = new QComboBox(this);
  inputCombo_->setMinimumContentsLength(16);
  outputCombo_->setMinimumContentsLength(16);
  inputCombo_->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
  outputCombo_->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);

  deviceIdSpin_ = new QSpinBox(this);
  deviceIdSpin_->setRange(0, 0x1F);
  deviceIdSpin_->setDisplayIntegerBase(16);
  deviceIdSpin_->setPrefix(QStringLiteral("0x"));
  deviceIdSpin_->setValue(0x00);

  toolbar->addWidget(labeledWidget(QStringLiteral("Input"), inputCombo_, toolbar));
  toolbar->addWidget(labeledWidget(QStringLiteral("Output"), outputCombo_, toolbar));
  toolbar->addWidget(labeledWidget(QStringLiteral("Device ID"), deviceIdSpin_, toolbar));

  refreshAction_ = toolbar->addAction(QStringLiteral("Refresh"), this, &MainWindow::onRefreshPorts);
  connectAction_ = toolbar->addAction(QStringLiteral("Connect"), this, &MainWindow::onConnect);
  toolbar->addSeparator();
  dumpAction_ = toolbar->addAction(QStringLiteral("Dump (RQ1)"), this, &MainWindow::onRequestAllPatches);
  dumpAction_->setToolTip(QStringLiteral("Request all patches (RQ1 Group A then Group B)"));
  listenAction_ = toolbar->addAction(QStringLiteral("Listen"));
  listenAction_->setCheckable(true);
  listenAction_->setToolTip(QStringLiteral("Listen for a panel bulk dump (stop manually)"));
  connect(listenAction_, &QAction::toggled, this, &MainWindow::onListenToggled);
  openAction_ = toolbar->addAction(QStringLiteral("Open file"), this, &MainWindow::onOpenFile);
  openAction_->setToolTip(QStringLiteral("Open a captured SysEx .bin"));

  auto* splitter = new QSplitter(Qt::Horizontal, this);
  splitter->setChildrenCollapsible(false);

  listPanel_ = new PatchListPanel(this);
  listPanel_->setMinimumWidth(200);
  splitter->addWidget(listPanel_);

  auto* right = new QWidget(this);
  auto* rightLayout = new QVBoxLayout(right);
  rightLayout->setContentsMargins(12, 12, 12, 12);
  rightLayout->setSpacing(10);

  auto* header = new QWidget(right);
  auto* headerLayout = new QHBoxLayout(header);
  headerLayout->setContentsMargins(0, 0, 0, 0);
  headerLayout->setSpacing(12);
  headerId_ = new QLabel(header);
  headerName_ = new QLabel(header);
  auto idFont = headerId_->font();
  idFont.setBold(true);
  idFont.setPointSizeF(idFont.pointSizeF() + 2);
  headerId_->setFont(idFont);
  headerId_->setMinimumWidth(48);
  auto nameFont = headerName_->font();
  nameFont.setPointSizeF(nameFont.pointSizeF() + 3);
  nameFont.setBold(true);
  headerName_->setFont(nameFont);
  headerLayout->addWidget(headerId_);
  headerLayout->addWidget(headerName_, 1);
  rightLayout->addWidget(header);

  chainWidget_ = new SignalChainWidget(right);
  rightLayout->addWidget(chainWidget_);

  editor_ = new EffectEditor(right);
  rightLayout->addWidget(editor_, 1);

  splitter->addWidget(right);
  splitter->setStretchFactor(0, 0);
  splitter->setStretchFactor(1, 1);
  splitter->setSizes({240, 860});
  setCentralWidget(splitter);

  logDock_ = new QDockWidget(QStringLiteral("MIDI log"), this);
  logDock_->setObjectName(QStringLiteral("midiLogDock"));
  logDock_->setAllowedAreas(Qt::BottomDockWidgetArea | Qt::TopDockWidgetArea);
  logView_ = new QPlainTextEdit(logDock_);
  logView_->setReadOnly(true);
  logView_->setMaximumBlockCount(5000);
  logView_->setMinimumHeight(80);
  logDock_->setWidget(logView_);
  addDockWidget(Qt::BottomDockWidgetArea, logDock_);
  resizeDocks({logDock_}, {160}, Qt::Vertical);

  auto* viewMenu = menuBar()->addMenu(QStringLiteral("&View"));
  viewMenu->addAction(logDock_->toggleViewAction());

  portStatus_ = new QLabel(this);
  statusBar()->addPermanentWidget(portStatus_);

  connect(listPanel_, &PatchListPanel::patchSelected, this, &MainWindow::onPatchSelected);
  connect(chainWidget_, &SignalChainWidget::slotSelected, this, &MainWindow::onChainSlotSelected);
  connect(chainWidget_, &SignalChainWidget::effectToggled, this, &MainWindow::onChainEffectToggled);
  connect(deviceIdSpin_, qOverload<int>(&QSpinBox::valueChanged), this, &MainWindow::onDeviceIdChanged);

  connect(midi_, &MidiService::portsChanged, this, [this]() {
    inputCombo_->clear();
    outputCombo_->clear();
    inputCombo_->addItems(midi_->inputPortNames());
    outputCombo_->addItems(midi_->outputPortNames());
    preferUsbMidi();
  });
  connect(midi_, &MidiService::sysExReceived, this, &MainWindow::onSysEx);
  connect(midi_, &MidiService::logMessage, this, &MainWindow::onLog);
  connect(midi_, &MidiService::midiError, this, &MainWindow::onError);
  connect(midi_, &MidiService::portsOpened, this, [this](bool, bool) {
    updateActions();
    updatePortStatus();
  });

  updateHeader(-1);
  updateActions();
  updatePortStatus();
  onRefreshPorts();
  appendLog(QStringLiteral("Ready. Open a capture, Dump (RQ1), or Listen for a panel dump."));
}

MainWindow::~MainWindow() = default;

bool MainWindow::openDumpFile(const QString& path)
{
  if (dumpPhase_ != DumpPhase::Idle)
    finishActivity(QStringLiteral("Ingest cancelled."), false);

  PatchBank loaded;
  std::string error;
  if (!loaded.loadFile(path.toStdString(), error)) {
    const auto msg = QStringLiteral("Failed to open %1: %2")
                         .arg(path, QString::fromStdString(error));
    appendLog(msg);
    statusBar()->showMessage(msg, 8000);
    return false;
  }

  bank_ = std::move(loaded);
  listPanel_->clearFilter();
  refreshLibrarian();
  listPanel_->selectPatch(0);

  const auto shape = bank_.shape() == PatchBank::IngestShape::PanelBulkDump
                         ? QStringLiteral("panel bulk dump")
                     : bank_.shape() == PatchBank::IngestShape::Rq1BulkDump
                         ? QStringLiteral("RQ1 bulk dump")
                         : QStringLiteral("unknown");
  const auto msg = QStringLiteral("Loaded %1 (%2, %3/128 patches)")
                       .arg(path, shape)
                       .arg(bank_.presentCount());
  appendLog(msg);
  statusBar()->showMessage(msg, 8000);
  return true;
}

void MainWindow::onRefreshPorts()
{
  midi_->refreshPorts();
}

void MainWindow::onConnect()
{
  if (dumpPhase_ != DumpPhase::Idle)
    finishActivity(QStringLiteral("Ingest cancelled."), false);
  sysexCount_ = 0;
  sysexBytes_ = 0;
  midi_->closePorts();
  midi_->setDeviceId(static_cast<std::uint8_t>(deviceIdSpin_->value()));
  midi_->openPorts(inputCombo_->currentText(), outputCombo_->currentText());
}

void MainWindow::sendGroupRequest(DumpPhase phase)
{
  dumpPhase_ = phase;
  dumpBuffer_.clear();
  dumpMessages_ = 0;
  dumpTimeout_->start();
  updateActions();
  const bool groupB = phase == DumpPhase::GroupB;
  appendLog(groupB ? QStringLiteral("Requesting Group B (64 patches)…")
                   : QStringLiteral("Requesting Group A (64 patches)…"));
  statusBar()->showMessage(groupB ? QStringLiteral("Dump Group B: 0/8192 bytes")
                                  : QStringLiteral("Dump Group A: 0/8192 bytes"));
  midi_->requestDataDump(
      groupB ? roland::kInternalGroupBAddress : roland::kInternalGroupAAddress,
      roland::kInternalGroupSize);
}

void MainWindow::finishActivity(const QString& detail, bool success)
{
  dumpTimeout_->stop();
  dumpPhase_ = DumpPhase::Idle;
  dumpBuffer_.clear();
  dumpMessages_ = 0;
  {
    QSignalBlocker blocker(listenAction_);
    listenAction_->setChecked(false);
  }
  updateActions();
  if (!detail.isEmpty()) {
    appendLog(detail);
    statusBar()->showMessage(detail, success ? 8000 : 12000);
  }
}

void MainWindow::stopListen(const QString& detail)
{
  finishActivity(detail, true);
}

void MainWindow::onRequestAllPatches()
{
  if (dumpPhase_ != DumpPhase::Idle)
    return;
  if (!midi_->isOutputOpen()) {
    onError(QStringLiteral("Connect an output port before Dump."));
    return;
  }
  bank_ = PatchBank{};
  listPanel_->clearFilter();
  refreshLibrarian();
  appendLog(QStringLiteral("Starting Group A + Group B RQ1 dump…"));
  sendGroupRequest(DumpPhase::GroupA);
}

void MainWindow::onListenToggled(bool on)
{
  if (!on) {
    if (dumpPhase_ == DumpPhase::Listening)
      stopListen(QStringLiteral("Listen stopped (%1/128 patches).").arg(bank_.presentCount()));
    return;
  }

  if (dumpPhase_ != DumpPhase::Idle) {
    QSignalBlocker blocker(listenAction_);
    listenAction_->setChecked(false);
    return;
  }
  if (!midi_->isInputOpen()) {
    QSignalBlocker blocker(listenAction_);
    listenAction_->setChecked(false);
    onError(QStringLiteral("Connect an input port before Listen."));
    return;
  }

  bank_ = PatchBank{};
  listPanel_->clearFilter();
  refreshLibrarian();
  dumpPhase_ = DumpPhase::Listening;
  dumpMessages_ = 0;
  updateActions();
  appendLog(QStringLiteral("Listening for panel bulk dump (0F nn 00). Stop when the device finishes."));
  statusBar()->showMessage(QStringLiteral("Listening: 0/128 patches"));
}

void MainWindow::onOpenFile()
{
  const auto path = QFileDialog::getOpenFileName(
      this,
      QStringLiteral("Open GP-16 dump"),
      lastOpenDir_,
      QStringLiteral("SysEx dumps (*.bin);;All files (*)"));
  if (path.isEmpty())
    return;
  lastOpenDir_ = QFileInfo(path).absolutePath();
  openDumpFile(path);
}

void MainWindow::onDumpTimeout()
{
  if (dumpPhase_ != DumpPhase::GroupA && dumpPhase_ != DumpPhase::GroupB)
    return;
  const auto group = dumpPhase_ == DumpPhase::GroupA ? QStringLiteral("A") : QStringLiteral("B");
  const int base = dumpPhase_ == DumpPhase::GroupB ? roland::kPatchesPerGroup : 0;
  if (!dumpBuffer_.empty()) {
    bank_.ingestGroupPayload(dumpBuffer_, base);
    refreshLibrarian();
  }
  finishActivity(
      QStringLiteral("Dump timeout on Group %1 after %2 payload bytes / %3 DT1 messages")
          .arg(group)
          .arg(static_cast<int>(dumpBuffer_.size()))
          .arg(dumpMessages_),
      false);
}

void MainWindow::onSysEx(const QByteArray& sysex)
{
  ++sysexCount_;
  sysexBytes_ += sysex.size();

  auto parsed = roland::parseDt1(asBytes(sysex));

  if (dumpPhase_ == DumpPhase::Listening) {
    if (parsed.valid && parsed.address.size() >= 2 && parsed.address[0] == 0x0F) {
      bank_.ingestPanelMessage(asBytes(sysex));
      ++dumpMessages_;
      refreshLibrarian();
      const int have = bank_.presentCount();
      const auto id = QString::fromStdString(Patch::displayIdFor(parsed.address[1]));
      const auto name = QString::fromStdString(roland::extractPatchName(parsed.data));
      appendLog(QStringLiteral("RX panel %1/128 %2 %3 (%4 B)")
                    .arg(have)
                    .arg(id, name)
                    .arg(sysex.size()));
      statusBar()->showMessage(QStringLiteral("Listening: %1/128 patches").arg(have));
      if (have >= PatchBank::kPatchCount) {
        stopListen(QStringLiteral("Listen complete (128 patches)."));
      }
    } else {
      appendLog(QStringLiteral("RX SysEx #%1 (%2 B) %3")
                    .arg(sysexCount_)
                    .arg(sysex.size())
                    .arg(toHex(sysex)));
    }
    return;
  }

  if ((dumpPhase_ == DumpPhase::GroupA || dumpPhase_ == DumpPhase::GroupB) && parsed.valid) {
    dumpBuffer_.insert(dumpBuffer_.end(), parsed.data.begin(), parsed.data.end());
    ++dumpMessages_;
    dumpTimeout_->start();
    const auto group = dumpPhase_ == DumpPhase::GroupA ? QStringLiteral("A") : QStringLiteral("B");
    appendLog(QStringLiteral("RX DT1 #%1 (%2 B, payload %3/%4, group %5)")
                  .arg(dumpMessages_)
                  .arg(sysex.size())
                  .arg(static_cast<int>(dumpBuffer_.size()))
                  .arg(roland::kGroupPayloadBytes)
                  .arg(group));
    statusBar()->showMessage(QStringLiteral("Dump Group %1: %2/%3 bytes")
                                 .arg(group)
                                 .arg(static_cast<int>(dumpBuffer_.size()))
                                 .arg(roland::kGroupPayloadBytes));

    if (static_cast<int>(dumpBuffer_.size()) >= roland::kGroupPayloadBytes) {
      if (dumpPhase_ == DumpPhase::GroupA) {
        bank_.ingestGroupPayload(dumpBuffer_, 0);
        refreshLibrarian();
        dumpPhase_ = DumpPhase::WaitGap;
        dumpTimeout_->stop();
        appendLog(QStringLiteral("Group A complete (%1 bytes).")
                      .arg(static_cast<int>(dumpBuffer_.size())));
        QTimer::singleShot(50, this, [this]() {
          if (dumpPhase_ == DumpPhase::WaitGap)
            sendGroupRequest(DumpPhase::GroupB);
        });
      } else if (dumpPhase_ == DumpPhase::GroupB) {
        bank_.ingestGroupPayload(dumpBuffer_, roland::kPatchesPerGroup);
        refreshLibrarian();
        finishActivity(
            QStringLiteral("Group B complete. Full dump received (%1/128 patches).")
                .arg(bank_.presentCount()),
            true);
      }
    }
    return;
  }

  QString nameHint;
  if (parsed.valid)
    nameHint = QString::fromStdString(roland::extractPatchName(parsed.data));
  appendLog(QStringLiteral("RX SysEx #%1 (%2 B) %3 %4")
                .arg(sysexCount_)
                .arg(sysex.size())
                .arg(toHex(sysex))
                .arg(nameHint.isEmpty() ? QString() : QStringLiteral("| %1").arg(nameHint)));
}

void MainWindow::onLog(const QString& message)
{
  appendLog(message);
}

void MainWindow::onError(const QString& message)
{
  appendLog(QStringLiteral("ERROR: %1").arg(message));
  statusBar()->showMessage(message, 12000);
  updatePortStatus();
}

void MainWindow::onDeviceIdChanged(int value)
{
  midi_->setDeviceId(static_cast<std::uint8_t>(value));
}

void MainWindow::onPatchSelected(int index)
{
  selectedIndex_ = index;
  updateHeader(index);
}

void MainWindow::onChainSlotSelected(int identity)
{
  if (selectedIndex_ < 0 || selectedIndex_ >= PatchBank::kPatchCount) {
    editor_->setPatch(nullptr);
    editor_->setIdentity(-1);
    return;
  }
  editor_->setPatch(&bank_.patchAt(selectedIndex_));
  editor_->setIdentity(identity);
}

void MainWindow::onChainEffectToggled(int identity, bool enabled)
{
  const auto& patch = currentPatch();
  const auto name = QString::fromStdString(
      Patch::effectName(identity, patch.blockB2Mode(), patch.isDistortion()));
  appendLog(QStringLiteral("%1 %2 (local model only; live edit arrives in Phase 6)")
                .arg(name, enabled ? QStringLiteral("enabled") : QStringLiteral("disabled")));
}

void MainWindow::refreshLibrarian()
{
  listPanel_->rebuild(bank_);
  const int index = listPanel_->currentPatchIndex();
  selectedIndex_ = index;
  updateHeader(index);
}

void MainWindow::updateHeader(int index)
{
  if (index < 0 || index >= PatchBank::kPatchCount) {
    headerId_->setText(QString());
    headerName_->setText(QStringLiteral("No patch selected"));
    chainWidget_->setPatch(nullptr);
    editor_->setPatch(nullptr);
    editor_->setIdentity(-1);
    return;
  }
  auto& patch = bank_.patchAt(index);
  headerId_->setText(QString::fromStdString(Patch::displayIdFor(index)));
  if (patch.isPresent() && !patch.name().empty())
    headerName_->setText(QString::fromStdString(patch.name()));
  else
    headerName_->setText(QStringLiteral("—"));
  chainWidget_->setPatch(&patch);
  editor_->setPatch(&patch);
  editor_->setIdentity(chainWidget_->selectedIdentity());
}

const Patch& MainWindow::currentPatch() const
{
  return bank_.patchAt(selectedIndex_);
}

void MainWindow::updateActions()
{
  const bool idle = dumpPhase_ == DumpPhase::Idle;
  const bool listening = dumpPhase_ == DumpPhase::Listening;
  dumpAction_->setEnabled(idle && midi_->isOutputOpen() && midi_->isInputOpen());
  listenAction_->setEnabled((idle || listening) && midi_->isInputOpen());
  openAction_->setEnabled(idle);
  connectAction_->setEnabled(true);
  refreshAction_->setEnabled(idle);
}

void MainWindow::updatePortStatus()
{
  portStatus_->setText(QStringLiteral("Input: %1 | Output: %2")
                           .arg(midi_->isInputOpen() ? QStringLiteral("open")
                                                     : QStringLiteral("closed"))
                           .arg(midi_->isOutputOpen() ? QStringLiteral("open")
                                                      : QStringLiteral("closed")));
}

void MainWindow::preferUsbMidi()
{
  const auto prefer = QStringLiteral("USB MIDI");
  for (int i = 0; i < inputCombo_->count(); ++i) {
    if (inputCombo_->itemText(i).contains(prefer, Qt::CaseInsensitive)) {
      inputCombo_->setCurrentIndex(i);
      break;
    }
  }
  for (int i = 0; i < outputCombo_->count(); ++i) {
    if (outputCombo_->itemText(i).contains(prefer, Qt::CaseInsensitive)) {
      outputCombo_->setCurrentIndex(i);
      break;
    }
  }
}

void MainWindow::appendLog(const QString& line)
{
  logView_->appendPlainText(line);
}

QString MainWindow::toHex(const QByteArray& data, int maxBytes)
{
  QString out;
  const int n = qMin(maxBytes, data.size());
  out.reserve(n * 3);
  for (int i = 0; i < n; ++i) {
    if (i)
      out += QLatin1Char(' ');
    out += QStringLiteral("%1")
               .arg(static_cast<unsigned char>(data[i]), 2, 16, QLatin1Char('0'))
               .toUpper();
  }
  if (data.size() > maxBytes)
    out += QStringLiteral(" …");
  return out;
}

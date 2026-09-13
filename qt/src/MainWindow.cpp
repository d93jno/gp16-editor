#include "MainWindow.h"
#include "MidiService.h"
#include "RolandSysex.h"

#include <QComboBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>

#include <span>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , midi_(new MidiService(this))
    , dumpTimeout_(new QTimer(this))
{
  setWindowTitle(QStringLiteral("GP-16 Editor (Qt + libremidi)"));
  resize(800, 560);

  dumpTimeout_->setSingleShot(true);
  dumpTimeout_->setInterval(8000);
  connect(dumpTimeout_, &QTimer::timeout, this, &MainWindow::onDumpTimeout);

  auto* central = new QWidget(this);
  setCentralWidget(central);
  auto* root = new QVBoxLayout(central);

  auto* form = new QFormLayout();
  inputCombo_ = new QComboBox(this);
  outputCombo_ = new QComboBox(this);
  deviceIdSpin_ = new QSpinBox(this);
  deviceIdSpin_->setRange(0, 0x1F);
  deviceIdSpin_->setDisplayIntegerBase(16);
  deviceIdSpin_->setPrefix(QStringLiteral("0x"));
  deviceIdSpin_->setValue(0x00);

  form->addRow(QStringLiteral("MIDI Input"), inputCombo_);
  form->addRow(QStringLiteral("MIDI Output"), outputCombo_);
  form->addRow(QStringLiteral("Device ID"), deviceIdSpin_);
  root->addLayout(form);

  auto* buttons = new QHBoxLayout();
  refreshButton_ = new QPushButton(QStringLiteral("Refresh ports"), this);
  openButton_ = new QPushButton(QStringLiteral("Open ports"), this);
  requestButton_ = new QPushButton(QStringLiteral("Request all patches (RQ1)"), this);
  requestButton_->setEnabled(false);
  buttons->addWidget(refreshButton_);
  buttons->addWidget(openButton_);
  buttons->addWidget(requestButton_);
  buttons->addStretch();
  root->addLayout(buttons);

  logView_ = new QPlainTextEdit(this);
  logView_->setReadOnly(true);
  logView_->setMaximumBlockCount(5000);
  root->addWidget(logView_, 1);

  statusLabel_ = new QLabel(QStringLiteral("Ready"), this);
  root->addWidget(statusLabel_);

  connect(refreshButton_, &QPushButton::clicked, this, &MainWindow::onRefreshPorts);
  connect(openButton_, &QPushButton::clicked, this, &MainWindow::onOpenPorts);
  connect(requestButton_, &QPushButton::clicked, this, &MainWindow::onRequestAllPatches);
  connect(deviceIdSpin_, qOverload<int>(&QSpinBox::valueChanged), this, &MainWindow::onDeviceIdChanged);

  connect(midi_, &MidiService::portsChanged, this, [this]() {
    inputCombo_->clear();
    outputCombo_->clear();
    inputCombo_->addItems(midi_->inputPortNames());
    outputCombo_->addItems(midi_->outputPortNames());

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
  });
  connect(midi_, &MidiService::sysExReceived, this, &MainWindow::onSysEx);
  connect(midi_, &MidiService::logMessage, this, &MainWindow::onLog);
  connect(midi_, &MidiService::midiError, this, &MainWindow::onError);
  connect(midi_, &MidiService::portsOpened, this, [this](bool inOk, bool outOk) {
    requestButton_->setEnabled(outOk && dumpPhase_ == DumpPhase::Idle);
    statusLabel_->setText(QStringLiteral("Input: %1 | Output: %2")
                              .arg(inOk ? QStringLiteral("open") : QStringLiteral("closed"))
                              .arg(outOk ? QStringLiteral("open") : QStringLiteral("closed")));
  });

  onRefreshPorts();
  appendLog(QStringLiteral("Qt + libremidi GP-16 skeleton ready."));
}

MainWindow::~MainWindow() = default;

void MainWindow::onRefreshPorts()
{
  midi_->refreshPorts();
}

void MainWindow::onOpenPorts()
{
  sysexCount_ = 0;
  sysexBytes_ = 0;
  finishDump(false, QString());
  midi_->setDeviceId(static_cast<std::uint8_t>(deviceIdSpin_->value()));
  midi_->openPorts(inputCombo_->currentText(), outputCombo_->currentText());
}

void MainWindow::sendGroupRequest(DumpPhase phase)
{
  dumpPhase_ = phase;
  dumpPayload_ = 0;
  dumpMessages_ = 0;
  dumpTimeout_->start();
  const bool groupB = phase == DumpPhase::GroupB;
  appendLog(groupB ? QStringLiteral("Requesting Group B (64 patches)…")
                   : QStringLiteral("Requesting Group A (64 patches)…"));
  midi_->requestDataDump(
      groupB ? roland::kInternalGroupBAddress : roland::kInternalGroupAAddress,
      roland::kInternalGroupSize);
}

void MainWindow::finishDump(bool success, const QString& detail)
{
  dumpTimeout_->stop();
  dumpPhase_ = DumpPhase::Idle;
  dumpPayload_ = 0;
  dumpMessages_ = 0;
  requestButton_->setEnabled(midi_->isOutputOpen());
  if (!detail.isEmpty()) {
    appendLog(detail);
    statusLabel_->setText(detail);
  }
  if (success)
    requestButton_->setEnabled(midi_->isOutputOpen());
}

void MainWindow::onRequestAllPatches()
{
  if (dumpPhase_ != DumpPhase::Idle)
    return;
  requestButton_->setEnabled(false);
  appendLog(QStringLiteral("Starting Group A + Group B RQ1 dump (3-byte address/size)…"));
  sendGroupRequest(DumpPhase::GroupA);
}

void MainWindow::onDumpTimeout()
{
  if (dumpPhase_ == DumpPhase::Idle)
    return;
  const auto group = dumpPhase_ == DumpPhase::GroupA ? QStringLiteral("A") : QStringLiteral("B");
  finishDump(
      false,
      QStringLiteral("Dump timeout on Group %1 after %2 payload bytes / %3 DT1 messages")
          .arg(group)
          .arg(dumpPayload_)
          .arg(dumpMessages_));
}

void MainWindow::onSysEx(const QByteArray& sysex)
{
  ++sysexCount_;
  sysexBytes_ += sysex.size();

  auto parsed = roland::parseDt1(
      std::span(reinterpret_cast<const std::uint8_t*>(sysex.constData()),
                static_cast<std::size_t>(sysex.size())));

  QString nameHint;
  if (parsed.valid)
    nameHint = QString::fromStdString(roland::extractPatchName(parsed.data));

  if (dumpPhase_ != DumpPhase::Idle && dumpPhase_ != DumpPhase::WaitGap && parsed.valid) {
    dumpPayload_ += static_cast<int>(parsed.data.size());
    ++dumpMessages_;
    dumpTimeout_->start();
    appendLog(QStringLiteral("RX DT1 #%1 (%2 B, payload %3, group %4/%5)%6")
                  .arg(dumpMessages_)
                  .arg(sysex.size())
                  .arg(parsed.data.size())
                  .arg(dumpPayload_)
                  .arg(roland::kGroupPayloadBytes)
                  .arg(nameHint.isEmpty() ? QString() : QStringLiteral(" | %1").arg(nameHint)));

    if (dumpPayload_ >= roland::kGroupPayloadBytes) {
      if (dumpPhase_ == DumpPhase::GroupA) {
        dumpPhase_ = DumpPhase::WaitGap;
        dumpTimeout_->stop();
        appendLog(QStringLiteral("Group A complete (%1 bytes).").arg(dumpPayload_));
        QTimer::singleShot(50, this, [this]() {
          if (dumpPhase_ == DumpPhase::WaitGap)
            sendGroupRequest(DumpPhase::GroupB);
        });
      } else if (dumpPhase_ == DumpPhase::GroupB) {
        finishDump(true, QStringLiteral("Group B complete. Full dump received."));
      }
    }
  } else {
    appendLog(QStringLiteral("RX SysEx #%1 (%2 B) %3 %4")
                  .arg(sysexCount_)
                  .arg(sysex.size())
                  .arg(toHex(sysex))
                  .arg(nameHint.isEmpty() ? QString() : QStringLiteral("| %1").arg(nameHint)));
  }

  statusLabel_->setText(QStringLiteral("SysEx: %1 msgs / %2 bytes")
                            .arg(sysexCount_)
                            .arg(sysexBytes_));
}

void MainWindow::onLog(const QString& message)
{
  appendLog(message);
}

void MainWindow::onError(const QString& message)
{
  appendLog(QStringLiteral("ERROR: %1").arg(message));
  statusLabel_->setText(message);
}

void MainWindow::onDeviceIdChanged(int value)
{
  midi_->setDeviceId(static_cast<std::uint8_t>(value));
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
    out += QStringLiteral("%1").arg(static_cast<unsigned char>(data[i]), 2, 16, QLatin1Char('0')).toUpper();
  }
  if (data.size() > maxBytes)
    out += QStringLiteral(" …");
  return out;
}

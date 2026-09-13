#pragma once

#include <QMainWindow>

#include <cstdint>

class MidiService;
class QComboBox;
class QSpinBox;
class QPushButton;
class QPlainTextEdit;
class QLabel;
class QTimer;

class MainWindow : public QMainWindow
{
  Q_OBJECT

public:
  explicit MainWindow(QWidget* parent = nullptr);
  ~MainWindow() override;

private slots:
  void onRefreshPorts();
  void onOpenPorts();
  void onRequestAllPatches();
  void onSysEx(const QByteArray& sysex);
  void onLog(const QString& message);
  void onError(const QString& message);
  void onDeviceIdChanged(int value);
  void onDumpTimeout();

private:
  enum class DumpPhase : std::uint8_t { Idle, GroupA, WaitGap, GroupB };

  void appendLog(const QString& line);
  void sendGroupRequest(DumpPhase phase);
  void finishDump(bool success, const QString& detail);
  static QString toHex(const QByteArray& data, int maxBytes = 48);

  MidiService* midi_ = nullptr;

  QComboBox* inputCombo_ = nullptr;
  QComboBox* outputCombo_ = nullptr;
  QSpinBox* deviceIdSpin_ = nullptr;
  QPushButton* refreshButton_ = nullptr;
  QPushButton* openButton_ = nullptr;
  QPushButton* requestButton_ = nullptr;
  QPlainTextEdit* logView_ = nullptr;
  QLabel* statusLabel_ = nullptr;
  QTimer* dumpTimeout_ = nullptr;

  DumpPhase dumpPhase_ = DumpPhase::Idle;
  int dumpPayload_ = 0;
  int dumpMessages_ = 0;

  int sysexCount_ = 0;
  int sysexBytes_ = 0;
};

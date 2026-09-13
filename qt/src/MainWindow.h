#pragma once

#include "PatchBank.h"

#include <QMainWindow>

#include <cstdint>
#include <vector>

class MidiService;
class PatchListPanel;
class QAction;
class QComboBox;
class QDockWidget;
class QLabel;
class QPlainTextEdit;
class QSpinBox;
class QStackedWidget;
class QTimer;

class MainWindow : public QMainWindow
{
  Q_OBJECT

public:
  explicit MainWindow(QWidget* parent = nullptr);
  ~MainWindow() override;

  bool openDumpFile(const QString& path);

private slots:
  void onRefreshPorts();
  void onConnect();
  void onRequestAllPatches();
  void onListenToggled(bool on);
  void onOpenFile();
  void onSysEx(const QByteArray& sysex);
  void onLog(const QString& message);
  void onError(const QString& message);
  void onDeviceIdChanged(int value);
  void onDumpTimeout();
  void onPatchSelected(int index);

private:
  enum class DumpPhase : std::uint8_t { Idle, GroupA, WaitGap, GroupB, Listening };

  void appendLog(const QString& line);
  void sendGroupRequest(DumpPhase phase);
  void finishActivity(const QString& detail, bool success);
  void stopListen(const QString& detail);
  void refreshLibrarian();
  void updateHeader(int index);
  void updateActions();
  void updatePortStatus();
  void preferUsbMidi();
  static QString toHex(const QByteArray& data, int maxBytes = 48);

  MidiService* midi_ = nullptr;
  PatchBank bank_;

  QComboBox* inputCombo_ = nullptr;
  QComboBox* outputCombo_ = nullptr;
  QSpinBox* deviceIdSpin_ = nullptr;
  QAction* refreshAction_ = nullptr;
  QAction* connectAction_ = nullptr;
  QAction* dumpAction_ = nullptr;
  QAction* listenAction_ = nullptr;
  QAction* openAction_ = nullptr;

  PatchListPanel* listPanel_ = nullptr;
  QLabel* headerId_ = nullptr;
  QLabel* headerName_ = nullptr;
  QStackedWidget* editorStack_ = nullptr;

  QDockWidget* logDock_ = nullptr;
  QPlainTextEdit* logView_ = nullptr;
  QLabel* portStatus_ = nullptr;
  QTimer* dumpTimeout_ = nullptr;

  DumpPhase dumpPhase_ = DumpPhase::Idle;
  std::vector<std::uint8_t> dumpBuffer_;
  int dumpMessages_ = 0;

  int sysexCount_ = 0;
  int sysexBytes_ = 0;
  int selectedIndex_ = -1;
  QString lastOpenDir_;
};

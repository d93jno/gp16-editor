#pragma once

#include "PatchBank.h"

#include <QMainWindow>

#include <cstdint>
#include <map>
#include <vector>

class EffectEditor;
class MidiService;
class Patch;
class PatchDisplayWidget;
class PatchListPanel;
class SignalChainWidget;
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
  bool importPatchFile(const QString& path, int destinationIndex);

  [[nodiscard]] MidiService* midiService() const { return midi_; }

private slots:
  void onRefreshPorts();
  void onConnect();
  void onRequestAllPatches();
  void onListenToggled(bool on);
  void onOpenFile();
  void onImportPatch();
  void onSysEx(const QByteArray& sysex);
  void onLog(const QString& message);
  void onError(const QString& message);
  void onDeviceIdChanged(int value);
  void onDumpTimeout();
  void onPatchSelected(int index);
  void onChainSlotSelected(int identity);
  void onChainEffectToggled(int identity, bool enabled);
  void onParameterEdited(int offset, int byteWidth, int value);
  void onEditCoalesceTick();

private:
  enum class DumpPhase : std::uint8_t { Idle, GroupA, WaitGap, GroupB, Listening };
  struct PendingParamEdit { int byteWidth; int value; };

  void appendLog(const QString& line);
  void sendGroupRequest(DumpPhase phase);
  void finishActivity(const QString& detail, bool success);
  void stopListen(const QString& detail);
  void cancelPendingEdits();
  void refreshLibrarian();
  void updateHeader(int index);
  void updateActions();
  void updatePortStatus();
  void preferUsbMidi();
  [[nodiscard]] QString defaultImportDir() const;
  void applyImportedPatch(Patch patch, int destinationIndex);
  [[nodiscard]] const Patch& currentPatch() const;
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
  QAction* importAction_ = nullptr;

  PatchListPanel* listPanel_ = nullptr;
  QStackedWidget* headerStack_ = nullptr;
  QLabel* headerId_ = nullptr;
  QLabel* headerName_ = nullptr;
  PatchDisplayWidget* display_ = nullptr;
  QAction* frontPanelAction_ = nullptr;
  SignalChainWidget* chainWidget_ = nullptr;
  EffectEditor* editor_ = nullptr;

  QDockWidget* logDock_ = nullptr;
  QPlainTextEdit* logView_ = nullptr;
  QLabel* portStatus_ = nullptr;
  QTimer* dumpTimeout_ = nullptr;
  QTimer* editCoalesceTimer_ = nullptr;
  std::map<int, PendingParamEdit> pendingEdits_;
  bool editBurstActive_ = false;

  DumpPhase dumpPhase_ = DumpPhase::Idle;
  std::vector<std::uint8_t> dumpBuffer_;
  int dumpMessages_ = 0;

  int sysexCount_ = 0;
  int sysexBytes_ = 0;
  int selectedIndex_ = -1;
  QString lastOpenDir_;
};

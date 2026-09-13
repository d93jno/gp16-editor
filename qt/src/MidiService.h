#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QByteArray>

#include <array>
#include <cstdint>
#include <memory>
#include <mutex>
#include <span>
#include <vector>

class MidiService : public QObject
{
  Q_OBJECT

public:
  explicit MidiService(QObject* parent = nullptr);
  ~MidiService() override;

  [[nodiscard]] QStringList inputPortNames() const;
  [[nodiscard]] QStringList outputPortNames() const;

  bool openPorts(const QString& inputName, const QString& outputName);

  void closePorts();

  [[nodiscard]] bool isInputOpen() const;
  [[nodiscard]] bool isOutputOpen() const;

  void setDeviceId(std::uint8_t id);
  [[nodiscard]] std::uint8_t deviceId() const;

  // Test-only: report the output port as open without a real MIDI backend, so
  // MainWindow's live-edit gating/coalescing can be exercised offline.
  // sendBytes() still has no backend to write to and reports an error via
  // midiError(), but sysExSent() still fires with the bytes that were built.
  void setTestOutputOpen(bool open);

  bool sendBytes(const std::vector<std::uint8_t>& bytes);
  bool sendSysEx(const QByteArray& sysex);

  bool requestDataDump(
      const std::array<std::uint8_t, 3>& address,
      const std::array<std::uint8_t, 3>& size);

  bool sendParameterChange(
      const std::array<std::uint8_t, 3>& address,
      std::uint8_t value);

  bool sendParameterChange(
      const std::array<std::uint8_t, 3>& address,
      std::span<const std::uint8_t> data);

public slots:
  void refreshPorts();

signals:
  void portsChanged();
  void portsOpened(bool inputOk, bool outputOk);
  void sysExReceived(const QByteArray& sysex);
  void sysExSent(const QByteArray& sysex);
  void midiError(const QString& message);
  void logMessage(const QString& message);

private:
  struct Impl;
  std::unique_ptr<Impl> impl_;

  void reassembleAndEmit(const std::vector<std::uint8_t>& bytes);

  mutable std::mutex mutex_;
  std::vector<std::uint8_t> sysexBuffer_;
  bool inSysex_ = false;

  std::uint8_t deviceId_ = 0x00;
  bool inputOpen_ = false;
  bool outputOpen_ = false;

  QStringList inputNames_;
  QStringList outputNames_;
};

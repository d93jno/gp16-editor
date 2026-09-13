#include "MidiService.h"
#include "RolandSysex.h"

#include <libremidi/libremidi.hpp>

#include <QMetaObject>

#include <algorithm>
#include <sstream>

namespace {

libremidi::API selectedApi()
{
#ifdef __linux__
  return libremidi::API::ALSA_RAW;
#else
  return libremidi::midi1::default_api();
#endif
}

QString portDisplayName(const libremidi::port_information& port)
{
  if (!port.display_name.empty())
    return QString::fromStdString(port.display_name);
  if (!port.port_name.empty())
    return QString::fromStdString(port.port_name);
  if (!port.device_name.empty())
    return QString::fromStdString(port.device_name);
  std::ostringstream oss;
  oss << port.display_name;
  return QString::fromStdString(oss.str());
}

QString errorText(const stdx::error& err)
{
  const auto ref = err.message();
  return QString::fromUtf8(ref.data(), static_cast<int>(ref.size()));
}

QString toHex(const std::vector<std::uint8_t>& data)
{
  QString out;
  out.reserve(static_cast<int>(data.size() * 3));
  for (std::size_t i = 0; i < data.size(); ++i) {
    if (i)
      out += QLatin1Char(' ');
    out += QStringLiteral("%1").arg(data[i], 2, 16, QLatin1Char('0')).toUpper();
  }
  return out;
}

} // namespace

struct MidiService::Impl {
  libremidi::API api = selectedApi();
  std::unique_ptr<libremidi::observer> observer;
  std::unique_ptr<libremidi::midi_in> midiIn;
  std::unique_ptr<libremidi::midi_out> midiOut;
  std::vector<libremidi::input_port> inputPorts;
  std::vector<libremidi::output_port> outputPorts;
};

MidiService::MidiService(QObject* parent)
    : QObject(parent)
    , impl_(std::make_unique<Impl>())
{
  refreshPorts();
}

MidiService::~MidiService()
{
  closePorts();
}

QStringList MidiService::inputPortNames() const
{
  std::scoped_lock lock(mutex_);
  return inputNames_;
}

QStringList MidiService::outputPortNames() const
{
  std::scoped_lock lock(mutex_);
  return outputNames_;
}

void MidiService::refreshPorts()
{
  QString apiName;
  int inputCount = 0;
  int outputCount = 0;

  {
    std::scoped_lock lock(mutex_);

    try {
      impl_->api = selectedApi();
      impl_->observer = std::make_unique<libremidi::observer>(
          libremidi::observer_configuration{
              .track_hardware = true,
              .track_virtual = true,
          },
          libremidi::observer_configuration_for(impl_->api));

      impl_->inputPorts = impl_->observer->get_input_ports();
      impl_->outputPorts = impl_->observer->get_output_ports();

      inputNames_.clear();
      for (const auto& p : impl_->inputPorts)
        inputNames_ << portDisplayName(p);

      outputNames_.clear();
      for (const auto& p : impl_->outputPorts)
        outputNames_ << portDisplayName(p);
    } catch (const std::exception& ex) {
      emit midiError(QStringLiteral("Port enumeration failed: %1").arg(ex.what()));
      return;
    }

    const auto apiNameRef = libremidi::get_api_display_name(impl_->api);
    apiName = QString::fromUtf8(apiNameRef.data(), static_cast<qsizetype>(apiNameRef.size()));
    inputCount = inputNames_.size();
    outputCount = outputNames_.size();
  }

  // Signals must be emitted with mutex_ released: connected slots (e.g. MainWindow's
  // portsChanged handler) call back into MidiService::inputPortNames()/outputPortNames(),
  // which lock the same non-recursive mutex — emitting while holding it deadlocks.
  emit portsChanged();
  emit logMessage(QStringLiteral("API: %1 — %2 inputs, %3 outputs")
                      .arg(apiName)
                      .arg(inputCount)
                      .arg(outputCount));
}

void MidiService::closePorts()
{
  std::scoped_lock lock(mutex_);
  impl_->midiIn.reset();
  impl_->midiOut.reset();
  inputOpen_ = false;
  outputOpen_ = false;
  inSysex_ = false;
  sysexBuffer_.clear();
}

bool MidiService::isInputOpen() const
{
  std::scoped_lock lock(mutex_);
  return inputOpen_;
}

bool MidiService::isOutputOpen() const
{
  std::scoped_lock lock(mutex_);
  return outputOpen_;
}

void MidiService::setDeviceId(std::uint8_t id)
{
  std::scoped_lock lock(mutex_);
  deviceId_ = id & 0x7F;
}

std::uint8_t MidiService::deviceId() const
{
  std::scoped_lock lock(mutex_);
  return deviceId_;
}

bool MidiService::openPorts(const QString& inputName, const QString& outputName)
{
  closePorts();

  bool inOk = false;
  bool outOk = false;

  {
    std::scoped_lock lock(mutex_);

    try {
    if (!inputName.isEmpty()) {
      const auto it = std::find_if(
          impl_->inputPorts.begin(), impl_->inputPorts.end(),
          [&](const libremidi::input_port& p) {
            return portDisplayName(p) == inputName;
          });
      if (it == impl_->inputPorts.end()) {
        emit midiError(QStringLiteral("Input port not found: %1").arg(inputName));
        return false;
      }

      impl_->midiIn = std::make_unique<libremidi::midi_in>(
          libremidi::input_configuration{
              .on_message =
                  [this](libremidi::message&& message) {
                    std::vector<std::uint8_t> bytes(message.begin(), message.end());
                    QMetaObject::invokeMethod(
                        this,
                        [this, bytes = std::move(bytes)]() { reassembleAndEmit(bytes); },
                        Qt::QueuedConnection);
                  },
              .on_error =
                  [this](std::string_view text, const libremidi::source_location&) {
                    const auto msg = QString::fromUtf8(text.data(), static_cast<int>(text.size()));
                    QMetaObject::invokeMethod(
                        this,
                        [this, msg]() { emit midiError(msg); },
                        Qt::QueuedConnection);
                  },
              .ignore_sysex = false,
              .ignore_timing = true,
              .ignore_sensing = true,
          },
          libremidi::midi_in_configuration_for(impl_->api));

      if (auto err = impl_->midiIn->open_port(*it); err != stdx::error{}) {
        emit midiError(QStringLiteral("Failed to open input: %1").arg(errorText(err)));
        impl_->midiIn.reset();
        return false;
      }
      inputOpen_ = true;
    }

    if (!outputName.isEmpty()) {
      const auto it = std::find_if(
          impl_->outputPorts.begin(), impl_->outputPorts.end(),
          [&](const libremidi::output_port& p) {
            return portDisplayName(p) == outputName;
          });
      if (it == impl_->outputPorts.end()) {
        emit midiError(QStringLiteral("Output port not found: %1").arg(outputName));
        return false;
      }

      impl_->midiOut = std::make_unique<libremidi::midi_out>(
          libremidi::output_configuration{},
          libremidi::midi_out_configuration_for(impl_->api));
      if (auto err = impl_->midiOut->open_port(*it); err != stdx::error{}) {
        emit midiError(QStringLiteral("Failed to open output: %1").arg(errorText(err)));
        impl_->midiOut.reset();
        inputOpen_ = false;
        impl_->midiIn.reset();
        return false;
      }
      outputOpen_ = true;
    }
  } catch (const std::exception& ex) {
    emit midiError(QStringLiteral("openPorts exception: %1").arg(ex.what()));
    impl_->midiIn.reset();
    impl_->midiOut.reset();
    inputOpen_ = false;
    outputOpen_ = false;
    return false;
  }

  inOk = inputOpen_;
  outOk = outputOpen_;
  }

  // Emitted after mutex_ is released — see the comment in refreshPorts().
  emit portsOpened(inOk, outOk);
  emit logMessage(QStringLiteral("Ports open — in: %1, out: %2")
                      .arg(inOk ? inputName : QStringLiteral("(none)"))
                      .arg(outOk ? outputName : QStringLiteral("(none)")));
  return inOk || outOk;
}

bool MidiService::sendBytes(const std::vector<std::uint8_t>& bytes)
{
  QString error;
  {
    std::scoped_lock lock(mutex_);
    if (!impl_->midiOut || !outputOpen_) {
      error = QStringLiteral("No MIDI output open");
    } else if (auto err = impl_->midiOut->send_message(bytes.data(), bytes.size());
               err != stdx::error{}) {
      error = QStringLiteral("send failed: %1").arg(errorText(err));
    }
  }

  // Emitted after mutex_ is released — see the comment in refreshPorts().
  if (!error.isEmpty()) {
    emit midiError(error);
    return false;
  }
  return true;
}

bool MidiService::sendSysEx(const QByteArray& sysex)
{
  std::vector<std::uint8_t> bytes(
      reinterpret_cast<const std::uint8_t*>(sysex.constData()),
      reinterpret_cast<const std::uint8_t*>(sysex.constData()) + sysex.size());
  return sendBytes(bytes);
}

bool MidiService::requestDataDump(
    const std::array<std::uint8_t, 3>& address,
    const std::array<std::uint8_t, 3>& size)
{
  std::uint8_t dev = 0;
  {
    std::scoped_lock lock(mutex_);
    dev = deviceId_;
  }
  const auto msg = roland::buildRequestData(dev, address, size);
  emit logMessage(QStringLiteral("TX RQ1 (%1 bytes): %2").arg(msg.size()).arg(toHex(msg)));
  return sendBytes(msg);
}

bool MidiService::sendParameterChange(
    const std::array<std::uint8_t, 3>& address,
    std::uint8_t value)
{
  const std::uint8_t data[1] = {value};
  return sendParameterChange(address, std::span<const std::uint8_t>(data, 1));
}

bool MidiService::sendParameterChange(
    const std::array<std::uint8_t, 3>& address,
    std::span<const std::uint8_t> data)
{
  std::uint8_t dev = 0;
  {
    std::scoped_lock lock(mutex_);
    dev = deviceId_;
  }
  const auto msg = roland::buildDataSet(dev, address, data);
  return sendBytes(msg);
}

void MidiService::reassembleAndEmit(const std::vector<std::uint8_t>& bytes)
{
  if (bytes.empty())
    return;

  if (bytes.front() == 0xF0 && bytes.back() == 0xF7) {
    QByteArray ba(
        reinterpret_cast<const char*>(bytes.data()),
        static_cast<int>(bytes.size()));
    emit sysExReceived(ba);
    return;
  }

  for (const auto b : bytes) {
    if (b == 0xF0) {
      inSysex_ = true;
      sysexBuffer_.clear();
      sysexBuffer_.push_back(b);
      continue;
    }
    if (!inSysex_)
      continue;

    sysexBuffer_.push_back(b);
    if (b == 0xF7) {
      inSysex_ = false;
      QByteArray ba(
          reinterpret_cast<const char*>(sysexBuffer_.data()),
          static_cast<int>(sysexBuffer_.size()));
      emit sysExReceived(ba);
      sysexBuffer_.clear();
    }
  }
}

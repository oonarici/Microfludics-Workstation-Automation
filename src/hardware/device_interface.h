/**
 * @file device_interface.h
 * @brief Abstract base class defining the interface for all hardware devices.
 * @author MWA Team
 * @date 2026-03-22
 *
 * This file defines the pure abstract DeviceInterface class, which all
 * hardware device implementations must inherit and implement.
 *
 * @copyright LGPL-3.0-or-later
 */

#pragma once

#include <QObject>
#include <QString>

namespace mwa::hardware {

/**
 * @class DeviceInterface
 * @brief Pure abstract base class for all hardware devices in the MWA system.
 *
 * DeviceInterface establishes the contract every hardware driver must
 * fulfil. It inherits from QObject to participate in Qt's signal/slot
 * mechanism and exposes signals for state transitions and error reporting.
 * Concrete subclasses must implement every pure virtual method.
 *
 * @note Because DeviceInterface inherits QObject it cannot be copied.
 *       Subclasses must accept a QObject parent pointer and forward it
 *       to this class's constructor.
 *
 * @see DeviceState
 * @see DeviceType
 */
class DeviceInterface : public QObject {
  Q_OBJECT

 public:
  /**
   * @enum DeviceState
   * @brief Represents the current connection state of a hardware device.
   */
  enum class DeviceState {
    kDisconnected,  ///< Device is not connected.
    kConnecting,    ///< Connection attempt is in progress.
    kConnected,     ///< Device is connected and ready.
    kError          ///< Device encountered an error.
  };
  Q_ENUM(DeviceState)

  /**
   * @enum DeviceType
   * @brief Identifies the category of a hardware device.
   */
  enum class DeviceType {
    kLed,              ///< LED illumination source.
    kPump,             ///< Syringe pump.
    kSignalGenerator,  ///< Signal/waveform generator.
    kNetworkAnalyzer,  ///< Network/impedance analyzer.
    kCamera,           ///< Imaging camera.
    kStage             ///< Motorized XYZ translation stage.
  };
  Q_ENUM(DeviceType)

  /**
   * @brief Virtual destructor.
   */
  ~DeviceInterface() override;

  /**
   * @brief Initiate an asynchronous connection to the physical device.
   *
   * Implementations should transition the device state to
   * DeviceState::kConnecting immediately and emit stateChanged(). On
   * success the state becomes DeviceState::kConnected; on failure it
   * becomes DeviceState::kError and errorOccurred() is emitted.
   */
  virtual void connectDevice() = 0;

  /**
   * @brief Initiate a graceful disconnection from the physical device.
   *
   * After calling this method the device state must eventually reach
   * DeviceState::kDisconnected and stateChanged() must be emitted.
   */
  virtual void disconnectDevice() = 0;

  /**
   * @brief Return a human-readable name identifying this device instance.
   *
   * @return A non-empty QString with the device display name.
   */
  [[nodiscard]] virtual QString deviceName() const = 0;

  /**
   * @brief Return the category of this device.
   *
   * @return The DeviceType enum value for this device.
   */
  [[nodiscard]] virtual DeviceType deviceType() const = 0;

  /**
   * @brief Return the current connection state of this device.
   *
   * @return The DeviceState enum value representing the current state.
   */
  [[nodiscard]] virtual DeviceState state() const = 0;

  /**
   * @brief Convenience query for whether the device is fully connected.
   *
   * @return @c true if the device state is DeviceState::kConnected.
   */
  [[nodiscard]] virtual bool isConnected() const = 0;

 signals:
  /**
   * @brief Emitted whenever the device connection state changes.
   *
   * @param new_state The updated DeviceState value.
   */
  void stateChanged(
      mwa::hardware::DeviceInterface::DeviceState new_state);

  /**
   * @brief Emitted when the device encounters an error.
   *
   * @param message A human-readable description of the error.
   */
  void errorOccurred(const QString& message);

 protected:
  /**
   * @brief Protected constructor — only concrete subclasses may call this.
   *
   * @param parent Optional QObject parent for Qt ownership management.
   */
  explicit DeviceInterface(QObject* parent = nullptr)
      : QObject(parent) {}
};

}  // namespace mwa::hardware

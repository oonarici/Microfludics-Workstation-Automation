/**
 * @file led_controller.h
 * @brief Hardware LED controller driver using QSerialPort.
 * @author MWA Team
 * @date 2026-03-25
 *
 * Implements the LedControllerInterface for real LED hardware connected
 * via a USB-to-serial adapter. All serial I/O is serialised through a
 * CommandQueue running on a dedicated worker thread.
 *
 * @copyright LGPL-3.0-or-later
 */

#pragma once

#include <QMutex>

#include <memory>

#include "hardware/command_queue.h"
#include "hardware/led/led_controller_interface.h"

class QSerialPort;

namespace mwa::hardware {

/**
 * @class LedController
 * @brief Hardware LED controller communicating over a serial port.
 *
 * Drives an LED illumination source through a simple ASCII serial
 * protocol over QSerialPort.  Every I/O operation is enqueued on an
 * internal CommandQueue so that commands are executed one at a time on
 * a dedicated worker thread, keeping the GUI thread responsive.
 *
 * The serial protocol is ASCII, \\r\\n terminated:
 * | Command          | Response |
 * |------------------|----------|
 * | ON\\r\\n          | OK\\r\\n  |
 * | OFF\\r\\n         | OK\\r\\n  |
 * | INT &lt;pct&gt;\\r\\n  | OK\\r\\n  |
 * | INT?\\r\\n        | &lt;pct&gt;\\r\\n |
 *
 * @note The QSerialPort is created lazily on the CommandQueue worker
 *       thread the first time connectDevice() is called.
 *
 * @see LedControllerInterface
 * @see CommandQueue
 */
class LedController : public LedControllerInterface {
  Q_OBJECT

 public:
  /**
   * @brief Construct a LedController.
   *
   * @param parent Optional QObject parent for Qt ownership management.
   */
  explicit LedController(QObject* parent = nullptr);

  /**
   * @brief Destructor — shuts down the command queue and releases the
   *        serial port.
   */
  ~LedController() override;

  // Non-copyable, non-movable.
  LedController(const LedController&) = delete;
  LedController& operator=(const LedController&) = delete;

  /**
   * @brief Set the serial port name before connecting.
   *
   * Must be called before connectDevice(). Has no effect while the
   * device is connected.
   *
   * @param name Platform-specific port identifier
   *             (e.g. "/dev/ttyUSB0", "COM3").
   */
  void setPortName(const QString& name);

  /**
   * @brief Return the currently configured serial port name.
   *
   * @return The port name string.
   */
  [[nodiscard]] QString portName() const;

  /**
   * @brief Set the serial baud rate before connecting.
   *
   * Must be called before connectDevice(). The default is 9600.
   *
   * @param baud Baud rate value (e.g. 9600, 115200).
   */
  void setBaudRate(qint32 baud);

  /**
   * @brief Return the currently configured baud rate.
   *
   * @return The baud rate in bits per second.
   */
  [[nodiscard]] qint32 baudRate() const;

  // -- DeviceInterface overrides ------------------------------------------

  /**
   * @brief Open the serial port and verify communication.
   *
   * Transitions to kConnecting immediately, then enqueues a command
   * that opens the port and sends a handshake query.  On success the
   * state becomes kConnected; on failure it becomes kError.
   */
  void connectDevice() override;

  /**
   * @brief Close the serial port and transition to kDisconnected.
   */
  void disconnectDevice() override;

  /**
   * @brief Return the display name for this device.
   *
   * @return The string "LED Controller" (or port-qualified variant).
   */
  [[nodiscard]] QString deviceName() const override;

  /**
   * @brief Return the current connection state (thread-safe).
   *
   * @return The current DeviceState value.
   */
  [[nodiscard]] DeviceState state() const override;

  /**
   * @brief Query whether the device is fully connected (thread-safe).
   *
   * @return @c true if the device state is DeviceState::kConnected.
   */
  [[nodiscard]] bool isConnected() const override;

  // -- LedControllerInterface overrides -----------------------------------

  /**
   * @brief Set the LED brightness by sending an "INT" command.
   *
   * The value is clamped to [0.0, 100.0] before transmission.
   *
   * @param percent Brightness level in percent.
   */
  void setIntensity(double percent) override;

  /**
   * @brief Return the cached LED brightness intensity (thread-safe).
   *
   * @return Current intensity as a percentage [0.0, 100.0].
   */
  [[nodiscard]] double intensity() const override;

  /**
   * @brief Send a power ON or OFF command to the LED.
   *
   * @param on @c true to power on, @c false to power off.
   */
  void setPowerOn(bool on) override;

  /**
   * @brief Query whether the LED is currently powered on (thread-safe).
   *
   * @return @c true if the LED is powered on.
   */
  [[nodiscard]] bool isPowerOn() const override;

 private:
  /**
   * @brief Thread-safe setter for the device state.
   *
   * Updates the cached state under the mutex and emits stateChanged().
   *
   * @param new_state The new device state.
   */
  void setState(DeviceState new_state);

  std::unique_ptr<CommandQueue> command_queue_;
  /// Created lazily on the worker thread; destroyed after shutdown().
  std::unique_ptr<QSerialPort> port_;

  mutable QMutex mutex_;                        ///< Guards cached state.
  DeviceState state_{DeviceState::kDisconnected};  ///< Cached state.
  double intensity_{0.0};                       ///< Cached intensity %.
  bool power_on_{false};                        ///< Cached power state.
  QString port_name_;                           ///< Serial port name.
  qint32 baud_rate_{9600};                      ///< Serial baud rate.

  /// Per-command timeout for serial I/O (milliseconds).
  static constexpr int kCommandTimeoutMs = 5000;
  /// Timeout when opening the serial port (milliseconds).
  static constexpr int kConnectTimeoutMs = 10000;
  /// Blocking wait for a serial response line (milliseconds).
  static constexpr int kResponseWaitMs = 3000;
};

}  // namespace mwa::hardware

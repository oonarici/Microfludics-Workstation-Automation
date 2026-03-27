/**
 * @file stage_controller.h
 * @brief Hardware XYZ stage controller driver using QSerialPort.
 * @author MWA Team
 * @date 2026-03-25
 *
 * Implements the StageControllerInterface for a real motorised XYZ
 * translation stage connected via a USB-to-serial adapter. All serial
 * I/O is serialised through a CommandQueue running on a dedicated
 * worker thread.
 *
 * @copyright LGPL-3.0-or-later
 */

#pragma once

#include <QMutex>

#include <memory>

#include "hardware/command_queue.h"
#include "hardware/stage/stage_controller_interface.h"

class QSerialPort;

namespace mwa::hardware {

/**
 * @class StageController
 * @brief Hardware XYZ stage controller communicating over a serial port.
 *
 * Drives a motorised XYZ translation stage through a simple ASCII
 * serial protocol over QSerialPort.  Every I/O operation is enqueued
 * on an internal CommandQueue so that commands are executed one at a
 * time on a dedicated worker thread.
 *
 * The serial protocol is ASCII, \\r\\n terminated:
 * | Command                         | Response               |
 * |---------------------------------|------------------------|
 * | HOME\\r\\n                       | OK\\r\\n                |
 * | MOVE &lt;x&gt; &lt;y&gt; &lt;z&gt;\\r\\n         | OK\\r\\n                |
 * | RMOVE &lt;dx&gt; &lt;dy&gt; &lt;dz&gt;\\r\\n     | OK\\r\\n                |
 * | STOP\\r\\n                       | OK\\r\\n                |
 * | SPEED &lt;mm/s&gt;\\r\\n              | OK\\r\\n                |
 * | POS?\\r\\n                       | &lt;x&gt; &lt;y&gt; &lt;z&gt;\\r\\n     |
 *
 * @note The QSerialPort is created lazily on the CommandQueue worker
 *       thread the first time connectDevice() is called.
 *
 * @see StageControllerInterface
 * @see CommandQueue
 */
class StageController : public StageControllerInterface {
  Q_OBJECT

 public:
  /**
   * @brief Construct a StageController.
   *
   * @param parent Optional QObject parent for Qt ownership management.
   */
  explicit StageController(QObject* parent = nullptr);

  /**
   * @brief Destructor — shuts down the command queue and releases the
   *        serial port.
   */
  ~StageController() override;

  // Non-copyable, non-movable.
  StageController(const StageController&) = delete;
  StageController& operator=(const StageController&) = delete;

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
   * Must be called before connectDevice(). The default is 115200.
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
   * that opens the port.  On success the state becomes kConnected; on
   * failure it becomes kError.
   */
  void connectDevice() override;

  /**
   * @brief Close the serial port and transition to kDisconnected.
   */
  void disconnectDevice() override;

  /**
   * @brief Return the display name for this device.
   *
   * @return The string "Stage Controller" (or port-qualified variant).
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

  // -- StageControllerInterface overrides ---------------------------------

  /**
   * @brief Send a HOME command to the stage.
   *
   * Emits homeComplete() and positionChanged() when the stage reaches
   * its home position.
   */
  void home() override;

  /**
   * @brief Send a MOVE command for absolute positioning.
   *
   * @param x Target X position in millimetres.
   * @param y Target Y position in millimetres.
   * @param z Target Z position in millimetres.
   */
  void moveAbsolute(double x, double y, double z) override;

  /**
   * @brief Send an RMOVE command for relative positioning.
   *
   * @param dx X offset in millimetres.
   * @param dy Y offset in millimetres.
   * @param dz Z offset in millimetres.
   */
  void moveRelative(double dx, double dy, double dz) override;

  /**
   * @brief Send a STOP command to halt all motion immediately.
   */
  void stopMotion() override;

  /**
   * @brief Return the cached X-axis position (thread-safe).
   *
   * @return X position in millimetres.
   */
  [[nodiscard]] double positionX() const override;

  /**
   * @brief Return the cached Y-axis position (thread-safe).
   *
   * @return Y position in millimetres.
   */
  [[nodiscard]] double positionY() const override;

  /**
   * @brief Return the cached Z-axis position (thread-safe).
   *
   * @return Z position in millimetres.
   */
  [[nodiscard]] double positionZ() const override;

  /**
   * @brief Send a SPEED command to set the travel speed.
   *
   * @param mm_per_s Travel speed in millimetres per second.
   */
  void setSpeed(double mm_per_s) override;

  /**
   * @brief Return the cached travel speed (thread-safe).
   *
   * @return Travel speed in millimetres per second.
   */
  [[nodiscard]] double speed() const override;

  /**
   * @brief Query whether the stage is currently moving (thread-safe).
   *
   * @return @c true if a move is in progress.
   */
  [[nodiscard]] bool isMoving() const override;

 private:
  /**
   * @brief Parse a position response of the form "x y z".
   *
   * @param response The trimmed response string.
   * @param x Output X value.
   * @param y Output Y value.
   * @param z Output Z value.
   * @return @c true if parsing succeeded.
   */
  static bool parsePosition(const QString& response,
                             double& x, double& y, double& z);

  /**
   * @brief Thread-safe setter for the device state.
   *
   * @param new_state The new device state.
   */
  void setState(DeviceState new_state);

  /**
   * @brief Query the stage for its current position and update the
   *        cache.
   *
   * Sends "POS?" and parses the response. Must be called from the
   * worker thread.
   */
  void queryPosition();

  std::unique_ptr<CommandQueue> command_queue_;
  /// Created lazily on the worker thread; destroyed after shutdown().
  std::unique_ptr<QSerialPort> port_;

  mutable QMutex mutex_;                        ///< Guards cached state.
  DeviceState state_{DeviceState::kDisconnected};  ///< Cached state.
  double position_x_{0.0};                      ///< Cached X position.
  double position_y_{0.0};                      ///< Cached Y position.
  double position_z_{0.0};                      ///< Cached Z position.
  double speed_{1.0};                           ///< Cached speed mm/s.
  bool is_moving_{false};                       ///< Cached motion flag.
  QString port_name_;                           ///< Serial port name.
  qint32 baud_rate_{115200};                    ///< Serial baud rate.

  /// Per-command timeout for serial I/O (milliseconds).
  static constexpr int kCommandTimeoutMs = 5000;
  /// Timeout for move commands that take longer (milliseconds).
  static constexpr int kMoveTimeoutMs = 30000;
  /// Timeout when opening the serial port (milliseconds).
  static constexpr int kConnectTimeoutMs = 10000;
  /// Blocking wait for a serial response line (milliseconds).
  static constexpr int kResponseWaitMs = 3000;
};

}  // namespace mwa::hardware

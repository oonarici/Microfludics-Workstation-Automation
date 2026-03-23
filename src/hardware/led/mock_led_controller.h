/**
 * @file mock_led_controller.h
 * @brief Mock implementation of the LED controller interface for testing.
 * @author MWA Team
 * @date 2026-03-22
 *
 * Provides a simulated LED controller that implements LedControllerInterface
 * without requiring real hardware. Suitable for GUI development and unit
 * testing.
 *
 * @copyright LGPL-3.0-or-later
 */

#pragma once

#include "hardware/led/led_controller_interface.h"

namespace mwa::hardware {

/**
 * @class MockLedController
 * @brief Simulated LED controller for hardware-free development and testing.
 *
 * Implements LedControllerInterface with in-memory state. The connect()
 * operation uses a 500 ms QTimer delay to simulate real hardware latency.
 * Intensity defaults to 0.0% and power defaults to off.
 *
 * @see LedControllerInterface
 */
class MockLedController : public LedControllerInterface {
  Q_OBJECT

 public:
  /**
   * @brief Construct a MockLedController.
   *
   * @param parent Optional QObject parent for Qt ownership management.
   */
  explicit MockLedController(QObject* parent = nullptr);

  /**
   * @brief Virtual destructor.
   */
  ~MockLedController() override;

  // DeviceInterface overrides

  /**
   * @brief Initiate a simulated 500 ms asynchronous connection.
   *
   * Transitions state to DeviceState::kConnecting immediately, then after
   * 500 ms transitions to DeviceState::kConnected and emits stateChanged().
   */
  void connectDevice() override;

  /**
   * @brief Disconnect from the simulated device immediately.
   *
   * Transitions state to DeviceState::kDisconnected and emits stateChanged().
   */
  void disconnectDevice() override;

  /**
   * @brief Return the display name for this mock device.
   *
   * @return The string "Mock LED Controller".
   */
  [[nodiscard]] QString deviceName() const override;

  /**
   * @brief Return the current connection state.
   *
   * @return The current DeviceState value.
   */
  [[nodiscard]] DeviceState state() const override;

  /**
   * @brief Query whether the device is fully connected.
   *
   * @return @c true if the device state is DeviceState::kConnected.
   */
  [[nodiscard]] bool isConnected() const override;

  // LedControllerInterface overrides

  /**
   * @brief Set the LED brightness intensity and emit intensityChanged().
   *
   * @param percent Brightness level in the range [0.0, 100.0].
   */
  void setIntensity(double percent) override;

  /**
   * @brief Return the current LED brightness intensity.
   *
   * @return Current intensity as a percentage in the range [0.0, 100.0].
   */
  [[nodiscard]] double intensity() const override;

  /**
   * @brief Set the LED power state and emit powerStateChanged().
   *
   * @param on @c true to power on the LED, @c false to power off.
   */
  void setPowerOn(bool on) override;

  /**
   * @brief Query whether the LED is currently powered on.
   *
   * @return @c true if the LED is powered on, @c false otherwise.
   */
  [[nodiscard]] bool isPowerOn() const override;

 private:
  /// Current connection state of the device.
  DeviceState state_;
  /// Current brightness intensity in percent [0.0, 100.0].
  double intensity_;
  /// Whether the LED is powered on.
  bool power_on_;
};

}  // namespace mwa::hardware

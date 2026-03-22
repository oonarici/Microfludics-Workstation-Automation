/**
 * @file mock_stage_controller.h
 * @brief Mock implementation of the XYZ stage controller interface.
 * @author MWA Team
 * @date 2026-03-22
 *
 * Provides a simulated XYZ stage controller that implements
 * StageControllerInterface without requiring real hardware. Suitable for
 * GUI development and unit testing.
 *
 * @copyright LGPL-3.0-or-later
 */

#pragma once

#include "hardware/stage/stage_controller_interface.h"

namespace mwa::hardware {

/**
 * @class MockStageController
 * @brief Simulated XYZ stage controller for hardware-free development.
 *
 * Implements StageControllerInterface with in-memory state. The connect()
 * operation uses a 500 ms QTimer delay to simulate real hardware latency.
 * Initial position is (0, 0, 0) mm and default speed is 1.0 mm/s.
 * moveAbsolute() and moveRelative() use a QTimer to simulate a 300 ms move
 * delay. home() resets to (0, 0, 0) with the same delay.
 *
 * @see StageControllerInterface
 */
class MockStageController : public StageControllerInterface {
  Q_OBJECT

 public:
  /**
   * @brief Construct a MockStageController.
   *
   * @param parent Optional QObject parent for Qt ownership management.
   */
  explicit MockStageController(QObject* parent = nullptr);

  /**
   * @brief Virtual destructor.
   */
  ~MockStageController() override;

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
   * @return The string "Mock Stage Controller".
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

  // StageControllerInterface overrides

  /**
   * @brief Perform a simulated home sequence with 300 ms delay.
   *
   * Sets position to (0, 0, 0) after the delay and emits homeComplete()
   * and positionChanged().
   */
  void home() override;

  /**
   * @brief Move to an absolute position after a simulated 300 ms delay.
   *
   * Emits positionChanged() and moveComplete() on arrival.
   *
   * @param x Target X position in millimetres.
   * @param y Target Y position in millimetres.
   * @param z Target Z position in millimetres.
   */
  void moveAbsolute(double x, double y, double z) override;

  /**
   * @brief Move by a relative offset after a simulated 300 ms delay.
   *
   * Emits positionChanged() and moveComplete() on arrival.
   *
   * @param dx X offset in millimetres.
   * @param dy Y offset in millimetres.
   * @param dz Z offset in millimetres.
   */
  void moveRelative(double dx, double dy, double dz) override;

  /**
   * @brief Immediately halt all stage motion.
   */
  void stopMotion() override;

  /**
   * @brief Return the current X-axis position.
   *
   * @return X position in millimetres.
   */
  [[nodiscard]] double positionX() const override;

  /**
   * @brief Return the current Y-axis position.
   *
   * @return Y position in millimetres.
   */
  [[nodiscard]] double positionY() const override;

  /**
   * @brief Return the current Z-axis position.
   *
   * @return Z position in millimetres.
   */
  [[nodiscard]] double positionZ() const override;

  /**
   * @brief Set the stage travel speed and emit speedChanged().
   *
   * @param mm_per_s Travel speed in millimetres per second.
   */
  void setSpeed(double mm_per_s) override;

  /**
   * @brief Return the current stage travel speed.
   *
   * @return Travel speed in millimetres per second.
   */
  [[nodiscard]] double speed() const override;

  /**
   * @brief Query whether the stage is currently moving.
   *
   * @return @c true if the stage is in motion, @c false otherwise.
   */
  [[nodiscard]] bool isMoving() const override;

 private:
  /// Current connection state of the device.
  DeviceState state_;
  /// Current X position in millimetres.
  double position_x_;
  /// Current Y position in millimetres.
  double position_y_;
  /// Current Z position in millimetres.
  double position_z_;
  /// Travel speed in millimetres per second.
  double speed_;
  /// Whether the stage is currently moving.
  bool is_moving_;
};

}  // namespace mwa::hardware

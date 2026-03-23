/**
 * @file mock_pump_controller.h
 * @brief Mock implementation of the syringe pump controller interface.
 * @author MWA Team
 * @date 2026-03-22
 *
 * Provides a simulated syringe pump controller that implements
 * PumpControllerInterface without requiring real hardware. Suitable for
 * GUI development and unit testing.
 *
 * @copyright LGPL-3.0-or-later
 */

#pragma once

#include "hardware/pump/pump_controller_interface.h"

namespace mwa::hardware {

/**
 * @class MockPumpController
 * @brief Simulated syringe pump controller for hardware-free development.
 *
 * Implements PumpControllerInterface with in-memory state. The connect()
 * operation uses a 500 ms QTimer delay to simulate real hardware latency.
 * Flow rate defaults to 10.0 µL/min, target volume to 100.0 µL, and
 * syringe position to 0.0 µL.
 *
 * @see PumpControllerInterface
 */
class MockPumpController : public PumpControllerInterface {
  Q_OBJECT

 public:
  /**
   * @brief Construct a MockPumpController.
   *
   * @param parent Optional QObject parent for Qt ownership management.
   */
  explicit MockPumpController(QObject* parent = nullptr);

  /**
   * @brief Virtual destructor.
   */
  ~MockPumpController() override;

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
   * @return The string "Mock Pump Controller".
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

  // PumpControllerInterface overrides

  /**
   * @brief Set the infusion flow rate and emit flowRateChanged().
   *
   * @param uL_per_min Flow rate in microlitres per minute.
   */
  void setFlowRate(double uL_per_min) override;

  /**
   * @brief Return the current infusion flow rate.
   *
   * @return Flow rate in microlitres per minute.
   */
  [[nodiscard]] double flowRate() const override;

  /**
   * @brief Set the target infusion volume.
   *
   * @param uL Target volume in microlitres.
   */
  void setTargetVolume(double uL) override;

  /**
   * @brief Return the current target infusion volume.
   *
   * @return Target volume in microlitres.
   */
  [[nodiscard]] double targetVolume() const override;

  /**
   * @brief Start an infusion run and emit infusionStarted().
   */
  void startInfusion() override;

  /**
   * @brief Stop an active infusion run and emit infusionStopped().
   */
  void stopInfusion() override;

  /**
   * @brief Retract the syringe plunger to refill (resets position to 0).
   */
  void refill() override;

  /**
   * @brief Return the current syringe plunger position.
   *
   * @return Plunger position in microlitres.
   */
  [[nodiscard]] double currentPosition() const override;

  /**
   * @brief Query whether the pump is actively infusing.
   *
   * @return @c true if the pump is currently infusing, @c false otherwise.
   */
  [[nodiscard]] bool isInfusing() const override;

 private:
  /// Current connection state of the device.
  DeviceState state_;
  /// Current flow rate in microlitres per minute.
  double flow_rate_;
  /// Target infusion volume in microlitres.
  double target_volume_;
  /// Current syringe plunger position in microlitres.
  double position_;
  /// Whether the pump is actively infusing.
  bool is_infusing_;
};

}  // namespace mwa::hardware

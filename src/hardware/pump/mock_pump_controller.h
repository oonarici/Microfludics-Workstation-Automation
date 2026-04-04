/**
 * @file mock_pump_controller.h
 * @brief Mock implementation of the syringe pump controller interface.
 * @author MWA Team
 * @date 2026-04-02
 *
 * Provides a simulated syringe pump controller that implements
 * PumpControllerInterface without requiring real hardware. Features
 * timed dispensing and refill simulation, syringe size configuration
 * with flow rate validation, and optional error injection. Suitable
 * for GUI development and unit testing.
 *
 * @copyright LGPL-3.0-or-later
 */

#pragma once

#include "hardware/pump/pump_controller_interface.h"

class QTimer;

namespace mwa::hardware {

/// Infusion/refill timer tick interval in milliseconds.
inline constexpr int kInfusionTickMs = 100;

/// Constant refill (aspirate) flow rate in µL/min.
inline constexpr double kRefillFlowRate = 50.0;

/// Minimum accepted flow rate in µL/min.
inline constexpr double kMinFlowRate = 0.001;

/// Default syringe volume in µL.
inline constexpr double kDefaultSyringeVolume = 250.0;

/// Default fallback maximum flow rate in µL/min for unknown sizes.
inline constexpr double kFallbackMaxFlowRate = 100.0;

/**
 * @class MockPumpController
 * @brief Simulated syringe pump controller for hardware-free
 *        development.
 *
 * Implements PumpControllerInterface with in-memory state. The
 * connect() operation uses a 500 ms QTimer delay to simulate real
 * hardware latency. After connecting, the syringe position is set
 * to the configured syringe volume (full syringe).
 *
 * During infusion the mock ticks a QTimer every @c kInfusionTickMs
 * milliseconds, decrementing the syringe position at the configured
 * flow rate. Infusion ends automatically when the target volume is
 * reached or the syringe empties. Refill operates similarly but
 * aspirates at @c kRefillFlowRate until the syringe is full.
 *
 * Flow rate validation enforces syringe-size-dependent maximum
 * rates. An optional error injection mode gives startInfusion() a
 * 10 % chance of failing with a simulated mechanical stall.
 *
 * @see PumpControllerInterface
 */
class MockPumpController : public PumpControllerInterface {
  Q_OBJECT

 public:
  /**
   * @brief Construct a MockPumpController.
   *
   * @param parent Optional QObject parent for Qt ownership
   *               management.
   */
  explicit MockPumpController(QObject* parent = nullptr);

  /**
   * @brief Virtual destructor.
   */
  ~MockPumpController() override;

  // ── DeviceInterface overrides ──────────────────────────────

  /**
   * @brief Initiate a simulated 500 ms asynchronous connection.
   *
   * Transitions state to DeviceState::kConnecting immediately,
   * then after 500 ms transitions to DeviceState::kConnected,
   * sets the syringe position to syringe_volume_ (full syringe),
   * and emits stateChanged().
   */
  void connectDevice() override;

  /**
   * @brief Disconnect from the simulated device immediately.
   *
   * Stops any active infusion or refill timer, then transitions
   * state to DeviceState::kDisconnected and emits stateChanged().
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
   * @return @c true if the device state is
   *         DeviceState::kConnected.
   */
  [[nodiscard]] bool isConnected() const override;

  // ── PumpControllerInterface overrides ──────────────────────

  /**
   * @brief Set the infusion flow rate with validation.
   *
   * Rejects values below @c kMinFlowRate or above the maximum
   * flow rate for the current syringe size, emitting
   * errorOccurred() without changing the stored rate. Also
   * rejects calls when the device is not connected.
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
   * @brief Start a timed infusion run.
   *
   * Starts a QTimer that decrements syringe position at the
   * configured flow rate. Emits infusionStarted() on success.
   * Infusion ends automatically when the target volume is
   * dispensed or the syringe empties. If error injection is
   * enabled, there is a 10 % chance of an immediate simulated
   * mechanical stall.
   */
  void startInfusion() override;

  /**
   * @brief Stop an active infusion run and emit
   *        infusionStopped().
   */
  void stopInfusion() override;

  /**
   * @brief Begin a timed syringe refill (aspirate).
   *
   * Aspirates at @c kRefillFlowRate until the syringe position
   * reaches syringe_volume_. Emits positionChanged() on each
   * tick and stops automatically when full.
   */
  void refill() override;

  /**
   * @brief Return the current syringe plunger position.
   *
   * @return Plunger position in microlitres (remaining volume).
   */
  [[nodiscard]] double currentPosition() const override;

  /**
   * @brief Query whether the pump is actively infusing.
   *
   * @return @c true if the pump is currently infusing, @c false
   *         otherwise.
   */
  [[nodiscard]] bool isInfusing() const override;

  // ── Mock-specific methods ──────────────────────────────────

  /**
   * @brief Set the syringe capacity.
   *
   * Configures the total syringe volume which determines the
   * maximum flow rate and the refill target. Common sizes are
   * 100, 250, 500, and 1000 µL.
   *
   * @param uL Syringe capacity in microlitres. Must be > 0.
   */
  void setSyringeVolume(double uL);

  /**
   * @brief Return the configured syringe capacity.
   *
   * @return Syringe volume in microlitres.
   */
  [[nodiscard]] double syringeVolume() const;

  /**
   * @brief Enable or disable simulated error injection.
   *
   * When enabled, startInfusion() has a 10 % probability of
   * emitting errorOccurred("Simulated mechanical stall") and
   * refusing to start.
   *
   * @param enable @c true to enable error injection, @c false
   *               to disable.
   */
  void setSimulateError(bool enable);

  /**
   * @brief Query whether error injection is enabled.
   *
   * @return @c true if error injection is active.
   */
  [[nodiscard]] bool simulateError() const;

  /**
   * @brief Return the maximum flow rate for the current syringe.
   *
   * Uses a lookup table for common syringe sizes (100, 250, 500,
   * 1000 µL) and linear interpolation for intermediate sizes.
   * Falls back to @c kFallbackMaxFlowRate for out-of-range
   * values.
   *
   * @return Maximum flow rate in µL/min.
   */
  [[nodiscard]] double maxFlowRateForSyringe() const;

  /**
   * @brief Query whether the pump is actively refilling.
   *
   * @return @c true if the pump is currently aspirating.
   */
  [[nodiscard]] bool isRefilling() const;

  /**
   * @brief Return the total volume dispensed in the current or
   *        most recent infusion run.
   *
   * @return Dispensed volume in microlitres.
   */
  [[nodiscard]] double dispensedVolume() const;

 private:
  /**
   * @brief Handle one infusion timer tick.
   *
   * Decrements position_, increments dispensed_volume_, and
   * checks for target-reached or syringe-empty conditions.
   */
  void onInfusionTick();

  /**
   * @brief Handle one refill timer tick.
   *
   * Increments position_ and checks for syringe-full condition.
   */
  void onRefillTick();

  /**
   * @brief Stop the infusion timer and reset infusion state.
   */
  void stopInfusionTimer();

  /**
   * @brief Stop the refill timer and reset refill state.
   */
  void stopRefillTimer();

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

  /// Timer driving infusion dispensing simulation.
  QTimer* infusion_timer_;
  /// Timer driving refill (aspirate) simulation.
  QTimer* refill_timer_;
  /// Configured syringe capacity in microlitres.
  double syringe_volume_;
  /// Volume dispensed in the current infusion run (µL).
  double dispensed_volume_;
  /// Whether the pump is actively refilling.
  bool is_refilling_;
  /// Whether simulated error injection is active.
  bool simulate_error_;
};

}  // namespace mwa::hardware

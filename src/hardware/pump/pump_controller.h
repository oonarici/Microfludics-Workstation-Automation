/**
 * @file pump_controller.h
 * @brief Hardware syringe pump driver skeleton for the Cetoni Nemesys system.
 * @author onuronarici
 * @date 2026-04-04
 *
 * Declares the PumpController class, which wraps the Cetoni QmixSDK C API
 * to drive Nemesys syringe pump modules over a CAN bus (USB-to-CAN adapter).
 * All SDK calls are serialised through a CommandQueue running on a dedicated
 * worker thread.
 *
 * This is a **header-only skeleton** — the .cpp implementation will be written
 * when the Cetoni hardware and QmixSDK become available.  The header is fully
 * documented so that the implementation can be written directly from the
 * Doxygen comments and the protocol specification in
 * `docs/protocols/cetoni_nemesys_spec.md`.
 *
 * @note QmixSDK is only available on Windows and Linux.  On macOS this file
 *       is included in the build (so it compiles), but no .cpp is compiled.
 *       The GUI always works against PumpControllerInterface*, so the mock is
 *       used on unsupported platforms.
 *
 * @see PumpControllerInterface
 * @see CommandQueue
 * @see MockPumpController
 *
 * @copyright LGPL-3.0-or-later
 */

#pragma once

#include <QMutex>
#include <QString>
#include <QTimer>

#include <cstdint>
#include <memory>

#include "hardware/command_queue.h"
#include "hardware/pump/pump_controller_interface.h"

namespace mwa::hardware {

/**
 * @brief Opaque SDK handle type matching Cetoni's `dev_hdl` (long long).
 *
 * Using a typedef keeps the header independent of the QmixSDK headers,
 * which may not be available at build time on all platforms.
 */
using QmixDeviceHandle = int64_t;

/// Invalid / uninitialised device handle sentinel.
inline constexpr QmixDeviceHandle kInvalidQmixHandle = -1;

/// Default polling interval (ms) for position updates during infusion.
inline constexpr int kPumpPollIntervalMs = 100;

/// Default per-command timeout for SDK operations (ms).
inline constexpr int kPumpCommandTimeoutMs = 5000;

/// Timeout for the multi-step connect sequence (ms).
inline constexpr int kPumpConnectTimeoutMs = 15000;

/**
 * @class PumpController
 * @brief Hardware syringe pump controller wrapping the Cetoni QmixSDK.
 *
 * Implements PumpControllerInterface for real Cetoni Nemesys pump hardware.
 * All QmixSDK calls (prefixed `LCB_*`, `LCP_*`) are serialised through an
 * internal CommandQueue so that only one SDK call is in flight at a time,
 * as required by the SDK's thread-safety model.
 *
 * ### Typical usage
 * @code
 *   auto pump = std::make_unique<PumpController>();
 *   pump->setConfigPath("/path/to/cetoni/config");
 *   pump->setPumpName("Nemesys_S_1");
 *   pump->setSyringeParams(2.3, 60.0);  // 2.3 mm inner dia, 60 mm stroke
 *   pump->connectDevice();               // async — emits stateChanged()
 * @endcode
 *
 * ### Connection workflow (enqueued on CommandQueue)
 * 1. `LCB_Open(config_path)` — open the CAN bus
 * 2. `LCB_Start()` — start bus communication
 * 3. `LCB_LookupPumpByName(pump_name)` — obtain device handle
 * 4. `LCP_SetSyringeParam(handle, inner_dia, stroke)` — configure syringe
 * 5. `LCP_SetVolumeUnit(handle, MICRO_LITRES)` — set volume unit
 * 6. `LCP_SetFlowUnit(handle, MICRO_LITRES_PER_MIN)` — set flow unit
 * 7. `LCP_Enable(handle, 1)` — enable the pump drive
 * 8. Transition to DeviceState::kConnected
 *
 * ### Disconnection workflow
 * 1. `LCP_StopPumping(handle)` — halt any active motion
 * 2. `LCP_Enable(handle, 0)` — disable the drive
 * 3. `LCB_Stop()` — stop bus communication
 * 4. `LCB_Close()` — close the bus
 * 5. Transition to DeviceState::kDisconnected
 *
 * @note Non-copyable, non-movable.
 *
 * @see PumpControllerInterface
 * @see CommandQueue
 * @see docs/protocols/cetoni_nemesys_spec.md
 */
class PumpController : public PumpControllerInterface {
  Q_OBJECT

 public:
  /**
   * @brief Construct a PumpController.
   *
   * The controller starts in DeviceState::kDisconnected.  Call
   * setConfigPath(), setPumpName(), and optionally setSyringeParams()
   * before connectDevice().
   *
   * @param parent Optional QObject parent for Qt ownership management.
   */
  explicit PumpController(QObject* parent = nullptr);

  /**
   * @brief Destructor — disconnects the device and shuts down the
   *        command queue.
   */
  ~PumpController() override;

  // Non-copyable, non-movable.
  PumpController(const PumpController&) = delete;
  PumpController& operator=(const PumpController&) = delete;
  PumpController(PumpController&&) = delete;
  PumpController& operator=(PumpController&&) = delete;

  // ── Configuration (call before connectDevice) ───────────────

  /**
   * @brief Set the path to the Cetoni device configuration directory.
   *
   * This directory is created by the Cetoni Elements software and
   * contains the bus and device description files needed by
   * `LCB_Open()`.  Must be set before connectDevice().
   *
   * @param path Absolute path to the Cetoni config directory.
   */
  void setConfigPath(const QString& path);

  /**
   * @brief Return the currently configured Cetoni config directory.
   *
   * @return The config path, or an empty string if not set.
   */
  [[nodiscard]] QString configPath() const;

  /**
   * @brief Set the name used to look up the pump on the CAN bus.
   *
   * The name must match one of the pump devices defined in the Cetoni
   * configuration (e.g. "Nemesys_S_1", "neMESYS_Low_Pressure_1").
   * Used by `LCB_LookupPumpByName()` during connection.
   *
   * @param name Pump device name from the Cetoni configuration.
   */
  void setPumpName(const QString& name);

  /**
   * @brief Return the currently configured pump device name.
   *
   * @return The pump name, or an empty string if not set.
   */
  [[nodiscard]] QString pumpName() const;

  /**
   * @brief Configure the syringe geometry for the pump drive.
   *
   * These dimensions are passed to `LCP_SetSyringeParam()` during
   * connection.  Correct syringe parameters are essential for accurate
   * flow rate and volume calculations by the SDK.
   *
   * Common syringe dimensions (Hamilton):
   * | Volume (µL) | Inner Diameter (mm) | Stroke (mm) |
   * |-------------|---------------------|-------------|
   * | 100         | 1.46                | 60.0        |
   * | 250         | 2.30                | 60.0        |
   * | 500         | 3.26                | 60.0        |
   * | 1000        | 4.61                | 60.0        |
   *
   * @param inner_diameter_mm Syringe inner diameter in millimetres.
   * @param stroke_mm         Syringe piston stroke length in millimetres.
   */
  void setSyringeParams(double inner_diameter_mm, double stroke_mm);

  /**
   * @brief Return the configured syringe inner diameter.
   *
   * @return Inner diameter in millimetres, or 0.0 if not set.
   */
  [[nodiscard]] double syringeInnerDiameter() const;

  /**
   * @brief Return the configured syringe stroke length.
   *
   * @return Stroke length in millimetres, or 0.0 if not set.
   */
  [[nodiscard]] double syringeStroke() const;

  // ── DeviceInterface overrides ───────────────────────────────

  /**
   * @brief Open the CAN bus and initialise the pump.
   *
   * Transitions immediately to DeviceState::kConnecting, then enqueues
   * the full connection sequence on the CommandQueue:
   *
   * 1. `LCB_Open(config_path_)`
   * 2. `LCB_Start()`
   * 3. `LCB_LookupPumpByName(pump_name_)` → store handle
   * 4. `LCP_SetSyringeParam(handle, inner_dia_, stroke_)`
   * 5. `LCP_SetVolumeUnit(handle, MICRO_LITRES)`
   * 6. `LCP_SetFlowUnit(handle, MICRO_LITRES_PER_MIN)`
   * 7. `LCP_Enable(handle, 1)`
   * 8. Transition to kConnected and emit stateChanged()
   *
   * If any SDK call returns an error code, the sequence aborts,
   * the state transitions to kError, and errorOccurred() is emitted
   * with the SDK error description.
   *
   * @pre setConfigPath() and setPumpName() must have been called.
   */
  void connectDevice() override;

  /**
   * @brief Gracefully disconnect from the pump and close the CAN bus.
   *
   * Enqueues the shutdown sequence on the CommandQueue:
   * 1. `LCP_StopPumping(handle)` — halt any active motion
   * 2. `LCP_Enable(handle, 0)` — disable the pump drive
   * 3. `LCB_Stop()` — stop bus communication
   * 4. `LCB_Close()` — release bus resources
   * 5. Transition to kDisconnected and emit stateChanged()
   */
  void disconnectDevice() override;

  /**
   * @brief Return the display name for this pump device.
   *
   * @return A string in the form "Pump Controller (<pump_name>)".
   */
  [[nodiscard]] QString deviceName() const override;

  /**
   * @brief Return the current connection state (thread-safe).
   *
   * @return The current DeviceState value, read under a mutex.
   */
  [[nodiscard]] DeviceState state() const override;

  /**
   * @brief Query whether the device is fully connected (thread-safe).
   *
   * @return @c true if the device state is DeviceState::kConnected.
   */
  [[nodiscard]] bool isConnected() const override;

  // ── PumpControllerInterface overrides ───────────────────────

  /**
   * @brief Set the infusion flow rate.
   *
   * Validates the rate against the syringe-dependent maximum and
   * caches it for the next startInfusion() call.  The QmixSDK does
   * not have a "set flow rate" command; instead the rate is passed
   * to `LCP_Dispense()` when infusion starts.
   *
   * Emits flowRateChanged() on success, or errorOccurred() if the
   * device is not connected or the rate is out of range.
   *
   * @param uL_per_min Flow rate in microlitres per minute.
   */
  void setFlowRate(double uL_per_min) override;

  /**
   * @brief Return the current infusion flow rate (thread-safe).
   *
   * @return Flow rate in microlitres per minute.
   */
  [[nodiscard]] double flowRate() const override;

  /**
   * @brief Set the target infusion volume.
   *
   * Caches the target volume for the next startInfusion() call.
   *
   * @param uL Target volume in microlitres.
   */
  void setTargetVolume(double uL) override;

  /**
   * @brief Return the current target infusion volume (thread-safe).
   *
   * @return Target volume in microlitres.
   */
  [[nodiscard]] double targetVolume() const override;

  /**
   * @brief Start dispensing at the cached flow rate and target volume.
   *
   * Enqueues `LCP_Dispense(handle, target_volume_, flow_rate_)` on
   * the CommandQueue.  On success, starts the position polling timer
   * and emits infusionStarted().
   *
   * The polling timer fires every @c kPumpPollIntervalMs milliseconds,
   * enqueuing a query that calls `LCP_GetFillLevel()` and
   * `LCP_IsPumping()`.  Each poll emits positionChanged() with the
   * current syringe fill level.  When `LCP_IsPumping()` returns false,
   * the timer stops and infusionStopped() is emitted.
   *
   * @pre Device must be connected.  Flow rate and target volume must
   *      have been set.
   */
  void startInfusion() override;

  /**
   * @brief Immediately stop the pump and halt infusion.
   *
   * Enqueues `LCP_StopPumping(handle)` on the CommandQueue, stops
   * the polling timer, and emits infusionStopped().
   */
  void stopInfusion() override;

  /**
   * @brief Aspirate to refill the syringe to maximum capacity.
   *
   * Enqueues `LCP_Aspirate(handle, max_volume, refill_rate)` on the
   * CommandQueue.  Starts the position polling timer to track refill
   * progress.  The refill completes automatically when the syringe
   * fill level reaches maximum.
   *
   * The refill flow rate is determined by the syringe size and is
   * typically set to a moderate rate to avoid damaging the syringe.
   */
  void refill() override;

  /**
   * @brief Return the current syringe fill level (thread-safe).
   *
   * The fill level is updated by the polling timer during infusion
   * or refill via `LCP_GetFillLevel()`.
   *
   * @return Syringe fill level in microlitres.
   */
  [[nodiscard]] double currentPosition() const override;

  /**
   * @brief Query whether the pump is actively dispensing (thread-safe).
   *
   * Updated by the polling timer via `LCP_IsPumping()`.
   *
   * @return @c true if the pump is currently infusing.
   */
  [[nodiscard]] bool isInfusing() const override;

 private:
  /**
   * @brief Thread-safe setter for the device state.
   *
   * Updates state_ under mutex_ and emits stateChanged().
   *
   * @param new_state The new device state.
   */
  void setState(DeviceState new_state);

  /**
   * @brief Handle one position polling timer tick.
   *
   * Enqueues a query on the CommandQueue that calls
   * `LCP_GetFillLevel()` and `LCP_IsPumping()`.  If the pump has
   * stopped, the timer is stopped and infusionStopped() is emitted.
   * Otherwise positionChanged() is emitted with the updated fill level.
   */
  void onPollTick();

  /**
   * @brief Start the position polling timer.
   *
   * Connects the timer to onPollTick() and starts it with
   * @c kPumpPollIntervalMs interval.
   */
  void startPolling();

  /**
   * @brief Stop the position polling timer.
   */
  void stopPolling();

  /// Serialises all QmixSDK calls onto a single worker thread.
  std::unique_ptr<CommandQueue> command_queue_;

  /// SDK device handle for the pump, obtained from LCB_LookupPumpByName().
  QmixDeviceHandle pump_handle_{kInvalidQmixHandle};

  /// Timer that polls fill level / pumping state during infusion/refill.
  QTimer poll_timer_;

  mutable QMutex mutex_;  ///< Guards all cached state below.

  // ── Cached state (read/written under mutex_) ─────────────────

  DeviceState state_{DeviceState::kDisconnected};  ///< Connection state.
  double flow_rate_{0.0};         ///< Cached flow rate in µL/min.
  double target_volume_{0.0};     ///< Cached target volume in µL.
  double position_{0.0};          ///< Cached syringe fill level in µL.
  bool is_infusing_{false};       ///< Whether the pump is actively dispensing.

  // ── Configuration (set before connectDevice) ──────────────────

  QString config_path_;           ///< Cetoni device configuration directory.
  QString pump_name_;             ///< Pump device name for bus lookup.
  double syringe_inner_dia_{0.0}; ///< Syringe inner diameter in mm.
  double syringe_stroke_{0.0};    ///< Syringe piston stroke in mm.
};

}  // namespace mwa::hardware

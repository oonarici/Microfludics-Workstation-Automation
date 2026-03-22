/**
 * @file pump_controller_interface.h
 * @brief Abstract interface for syringe pump controllers.
 * @author MWA Team
 * @date 2026-03-22
 *
 * Defines the pure abstract PumpControllerInterface that all syringe pump
 * controller implementations must fulfil. Extends DeviceInterface with
 * pump-specific flow rate, volume, and infusion controls.
 *
 * @copyright LGPL-3.0-or-later
 */

#pragma once

#include "hardware/device_interface.h"

namespace mwa::hardware {

/**
 * @class PumpControllerInterface
 * @brief Pure abstract interface for syringe pump controllers.
 *
 * Extends DeviceInterface with syringe pump-specific pure virtual methods
 * for controlling flow rate, target volume, and infusion state. All concrete
 * pump controller implementations must inherit this interface.
 *
 * @see DeviceInterface
 * @see MockPumpController
 */
class PumpControllerInterface : public DeviceInterface {
  Q_OBJECT

 public:
  /**
   * @brief Protected constructor — only concrete subclasses may call this.
   *
   * @param parent Optional QObject parent for Qt ownership management.
   */
  explicit PumpControllerInterface(QObject* parent = nullptr)
      : DeviceInterface(parent) {}

  /**
   * @brief Virtual destructor.
   */
  ~PumpControllerInterface() override = default;

  /**
   * @brief Return the category of this device.
   *
   * @return DeviceType::kPump for all pump controller implementations.
   */
  [[nodiscard]] DeviceType deviceType() const override {
    return DeviceType::kPump;
  }

  /**
   * @brief Set the infusion flow rate.
   *
   * @param uL_per_min Flow rate in microlitres per minute.
   */
  virtual void setFlowRate(double uL_per_min) = 0;

  /**
   * @brief Return the current infusion flow rate.
   *
   * @return Flow rate in microlitres per minute.
   */
  [[nodiscard]] virtual double flowRate() const = 0;

  /**
   * @brief Set the target infusion volume.
   *
   * @param uL Target volume in microlitres.
   */
  virtual void setTargetVolume(double uL) = 0;

  /**
   * @brief Return the current target infusion volume.
   *
   * @return Target volume in microlitres.
   */
  [[nodiscard]] virtual double targetVolume() const = 0;

  /**
   * @brief Start an infusion run.
   *
   * Emits infusionStarted() when the pump begins infusing.
   */
  virtual void startInfusion() = 0;

  /**
   * @brief Stop an active infusion run.
   *
   * Emits infusionStopped() when the pump halts.
   */
  virtual void stopInfusion() = 0;

  /**
   * @brief Retract the syringe plunger to refill.
   */
  virtual void refill() = 0;

  /**
   * @brief Return the current syringe plunger position.
   *
   * @return Plunger position expressed as the remaining volume in microlitres.
   */
  [[nodiscard]] virtual double currentPosition() const = 0;

  /**
   * @brief Query whether the pump is actively infusing.
   *
   * @return @c true if the pump is currently infusing, @c false otherwise.
   */
  [[nodiscard]] virtual bool isInfusing() const = 0;

 signals:
  /**
   * @brief Emitted when the flow rate changes.
   *
   * @param uL_per_min The new flow rate in microlitres per minute.
   */
  void flowRateChanged(double uL_per_min);

  /**
   * @brief Emitted when the syringe position changes.
   *
   * @param uL The new plunger position in microlitres.
   */
  void positionChanged(double uL);

  /**
   * @brief Emitted when an infusion run begins.
   */
  void infusionStarted();

  /**
   * @brief Emitted when an infusion run ends.
   */
  void infusionStopped();
};

}  // namespace mwa::hardware

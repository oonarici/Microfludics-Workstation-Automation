/**
 * @file stage_controller_interface.h
 * @brief Abstract interface for motorized XYZ translation stage controllers.
 * @author MWA Team
 * @date 2026-03-22
 *
 * Defines the pure abstract StageControllerInterface that all XYZ stage
 * controller implementations must fulfil. Extends DeviceInterface with
 * axis-specific motion control, homing, and speed management.
 *
 * @copyright LGPL-3.0-or-later
 */

#pragma once

#include "hardware/device_interface.h"

namespace mwa::hardware {

/**
 * @class StageControllerInterface
 * @brief Pure abstract interface for motorized XYZ translation stage
 *        controllers.
 *
 * Extends DeviceInterface with stage-specific pure virtual methods for
 * absolute and relative motion, homing, speed control, and position
 * queries. All concrete stage controller implementations must inherit
 * this interface.
 *
 * @see DeviceInterface
 * @see MockStageController
 */
class StageControllerInterface : public DeviceInterface {
  Q_OBJECT

 public:
  /**
   * @enum Axis
   * @brief Identifies a single translation axis on the XYZ stage.
   */
  enum class Axis {
    kX,  ///< The X translation axis.
    kY,  ///< The Y translation axis.
    kZ   ///< The Z translation axis.
  };
  Q_ENUM(Axis)

  /**
   * @brief Constructor — forwards the QObject parent to DeviceInterface.
   *
   * @param parent Optional QObject parent for Qt ownership management.
   */
  explicit StageControllerInterface(QObject* parent = nullptr)
      : DeviceInterface(parent) {}

  /**
   * @brief Virtual destructor.
   */
  ~StageControllerInterface() override = default;

  /**
   * @brief Return the category of this device.
   *
   * @return DeviceType::kStage for all stage controller implementations.
   */
  [[nodiscard]] DeviceType deviceType() const override {
    return DeviceType::kStage;
  }

  /**
   * @brief Perform an asynchronous home sequence on all axes.
   *
   * Emits homeComplete() when all axes have reached their home positions.
   */
  virtual void home() = 0;

  /**
   * @brief Move to an absolute position in XYZ space.
   *
   * Emits positionChanged() as the stage moves and moveComplete() on arrival.
   *
   * @param x Target X position in millimetres.
   * @param y Target Y position in millimetres.
   * @param z Target Z position in millimetres.
   */
  virtual void moveAbsolute(double x, double y, double z) = 0;

  /**
   * @brief Move by a relative offset in XYZ space.
   *
   * Emits positionChanged() as the stage moves and moveComplete() on arrival.
   *
   * @param dx X offset in millimetres.
   * @param dy Y offset in millimetres.
   * @param dz Z offset in millimetres.
   */
  virtual void moveRelative(double dx, double dy, double dz) = 0;

  /**
   * @brief Immediately halt all stage motion.
   */
  virtual void stopMotion() = 0;

  /**
   * @brief Return the current X-axis position.
   *
   * @return X position in millimetres.
   */
  [[nodiscard]] virtual double positionX() const = 0;

  /**
   * @brief Return the current Y-axis position.
   *
   * @return Y position in millimetres.
   */
  [[nodiscard]] virtual double positionY() const = 0;

  /**
   * @brief Return the current Z-axis position.
   *
   * @return Z position in millimetres.
   */
  [[nodiscard]] virtual double positionZ() const = 0;

  /**
   * @brief Set the stage travel speed.
   *
   * @param mm_per_s Travel speed in millimetres per second.
   */
  virtual void setSpeed(double mm_per_s) = 0;

  /**
   * @brief Return the current stage travel speed.
   *
   * @return Travel speed in millimetres per second.
   */
  [[nodiscard]] virtual double speed() const = 0;

  /**
   * @brief Query whether the stage is currently moving.
   *
   * @return @c true if the stage is in motion, @c false otherwise.
   */
  [[nodiscard]] virtual bool isMoving() const = 0;

 signals:
  /**
   * @brief Emitted whenever the stage position changes.
   *
   * @param x Current X position in millimetres.
   * @param y Current Y position in millimetres.
   * @param z Current Z position in millimetres.
   */
  void positionChanged(double x, double y, double z);

  /**
   * @brief Emitted when a home sequence completes.
   */
  void homeComplete();

  /**
   * @brief Emitted when a move command reaches its target position.
   */
  void moveComplete();

  /**
   * @brief Emitted when the travel speed changes.
   *
   * @param mm_per_s The new travel speed in millimetres per second.
   */
  void speedChanged(double mm_per_s);
};

}  // namespace mwa::hardware

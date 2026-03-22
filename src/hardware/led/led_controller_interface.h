/**
 * @file led_controller_interface.h
 * @brief Abstract interface for LED illumination source controllers.
 * @author MWA Team
 * @date 2026-03-22
 *
 * Defines the pure abstract LedControllerInterface that all LED controller
 * implementations must fulfil. Extends DeviceInterface with LED-specific
 * intensity and power controls.
 *
 * @copyright LGPL-3.0-or-later
 */

#pragma once

#include "hardware/device_interface.h"

namespace mwa::hardware {

/**
 * @class LedControllerInterface
 * @brief Pure abstract interface for LED illumination source controllers.
 *
 * Extends DeviceInterface with LED-specific pure virtual methods for
 * controlling brightness intensity and power state. All concrete LED
 * controller implementations must inherit this interface.
 *
 * @see DeviceInterface
 * @see MockLedController
 */
class LedControllerInterface : public DeviceInterface {
  Q_OBJECT

 public:
  /**
   * @brief Protected constructor — only concrete subclasses may call this.
   *
   * @param parent Optional QObject parent for Qt ownership management.
   */
  explicit LedControllerInterface(QObject* parent = nullptr)
      : DeviceInterface(parent) {}

  /**
   * @brief Virtual destructor.
   */
  ~LedControllerInterface() override = default;

  /**
   * @brief Return the category of this device.
   *
   * @return DeviceType::kLed for all LED controller implementations.
   */
  [[nodiscard]] DeviceType deviceType() const override {
    return DeviceType::kLed;
  }

  /**
   * @brief Set the LED brightness intensity.
   *
   * @param percent Brightness level in the range [0.0, 100.0].
   */
  virtual void setIntensity(double percent) = 0;

  /**
   * @brief Return the current LED brightness intensity.
   *
   * @return Current intensity as a percentage in the range [0.0, 100.0].
   */
  [[nodiscard]] virtual double intensity() const = 0;

  /**
   * @brief Set the LED power state.
   *
   * @param on @c true to power on the LED, @c false to power off.
   */
  virtual void setPowerOn(bool on) = 0;

  /**
   * @brief Query whether the LED is currently powered on.
   *
   * @return @c true if the LED is powered on, @c false otherwise.
   */
  [[nodiscard]] virtual bool isPowerOn() const = 0;

 signals:
  /**
   * @brief Emitted when the LED intensity changes.
   *
   * @param percent The new intensity value in the range [0.0, 100.0].
   */
  void intensityChanged(double percent);

  /**
   * @brief Emitted when the LED power state changes.
   *
   * @param on @c true if the LED is now powered on, @c false if powered off.
   */
  void powerStateChanged(bool on);
};

}  // namespace mwa::hardware

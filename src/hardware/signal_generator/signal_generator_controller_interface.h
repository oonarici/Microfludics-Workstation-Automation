/**
 * @file signal_generator_controller_interface.h
 * @brief Abstract interface for signal/waveform generator controllers.
 * @author MWA Team
 * @date 2026-03-22
 *
 * Defines the pure abstract SignalGeneratorControllerInterface that all
 * signal generator controller implementations must fulfil. Extends
 * DeviceInterface with waveform, frequency, amplitude, and sweep controls.
 *
 * @copyright LGPL-3.0-or-later
 */

#pragma once

#include "hardware/device_interface.h"

namespace mwa::hardware {

/**
 * @class SignalGeneratorControllerInterface
 * @brief Pure abstract interface for signal/waveform generator controllers.
 *
 * Extends DeviceInterface with signal-generator-specific pure virtual methods
 * for controlling output frequency, amplitude, waveform shape, and frequency
 * sweep configuration. All concrete signal generator implementations must
 * inherit this interface.
 *
 * @see DeviceInterface
 * @see MockSignalGeneratorController
 */
class SignalGeneratorControllerInterface : public DeviceInterface {
  Q_OBJECT

 public:
  /**
   * @enum Waveform
   * @brief Output waveform shapes supported by the signal generator.
   */
  enum class Waveform {
    kSine,      ///< Sinusoidal waveform.
    kSquare,    ///< Square waveform.
    kTriangle   ///< Triangular waveform.
  };
  Q_ENUM(Waveform)

  /**
   * @brief Protected constructor — only concrete subclasses may call this.
   *
   * @param parent Optional QObject parent for Qt ownership management.
   */
  explicit SignalGeneratorControllerInterface(QObject* parent = nullptr)
      : DeviceInterface(parent) {}

  /**
   * @brief Virtual destructor.
   */
  ~SignalGeneratorControllerInterface() override = default;

  /**
   * @brief Return the category of this device.
   *
   * @return DeviceType::kSignalGenerator for all signal generator
   *         implementations.
   */
  [[nodiscard]] DeviceType deviceType() const override {
    return DeviceType::kSignalGenerator;
  }

  /**
   * @brief Set the output frequency.
   *
   * @param hz Output frequency in hertz.
   */
  virtual void setFrequency(double hz) = 0;

  /**
   * @brief Return the current output frequency.
   *
   * @return Output frequency in hertz.
   */
  [[nodiscard]] virtual double frequency() const = 0;

  /**
   * @brief Set the output amplitude.
   *
   * @param volts Peak amplitude in volts.
   */
  virtual void setAmplitude(double volts) = 0;

  /**
   * @brief Return the current output amplitude.
   *
   * @return Peak amplitude in volts.
   */
  [[nodiscard]] virtual double amplitude() const = 0;

  /**
   * @brief Set the output waveform shape.
   *
   * @param waveform The desired Waveform value.
   */
  virtual void setWaveform(Waveform waveform) = 0;

  /**
   * @brief Return the current output waveform shape.
   *
   * @return The current Waveform value.
   */
  [[nodiscard]] virtual Waveform waveform() const = 0;

  /**
   * @brief Enable or disable the signal output.
   *
   * @param enabled @c true to enable the output, @c false to disable.
   */
  virtual void setOutputEnabled(bool enabled) = 0;

  /**
   * @brief Query whether the signal output is currently enabled.
   *
   * @return @c true if the output is enabled, @c false otherwise.
   */
  [[nodiscard]] virtual bool isOutputEnabled() const = 0;

  /**
   * @brief Configure a frequency sweep.
   *
   * @param start_hz  Start frequency of the sweep in hertz.
   * @param stop_hz   Stop frequency of the sweep in hertz.
   * @param step_hz   Frequency step size in hertz.
   */
  virtual void configureSweep(
      double start_hz, double stop_hz, double step_hz) = 0;

 signals:
  /**
   * @brief Emitted when the output frequency changes.
   *
   * @param hz The new frequency in hertz.
   */
  void frequencyChanged(double hz);

  /**
   * @brief Emitted when the output amplitude changes.
   *
   * @param volts The new peak amplitude in volts.
   */
  void amplitudeChanged(double volts);

  /**
   * @brief Emitted when the output waveform shape changes.
   *
   * @param waveform The new Waveform value.
   */
  void waveformChanged(
      mwa::hardware::SignalGeneratorControllerInterface::Waveform waveform);

  /**
   * @brief Emitted when the output enable state changes.
   *
   * @param enabled @c true if the output is now enabled, @c false otherwise.
   */
  void outputStateChanged(bool enabled);
};

}  // namespace mwa::hardware

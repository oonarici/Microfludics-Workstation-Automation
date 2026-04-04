/**
 * @file signal_generator_controller.h
 * @brief Hardware signal generator driver skeleton for SCPI-based instruments.
 * @author onuronarici
 * @date 2026-04-04
 *
 * Declares the SignalGeneratorController class, which drives SCPI-based
 * signal/waveform generators (Rigol DG1000Z, Keysight 33500B, Tektronix
 * AFG1022 and compatible models) over a serial or USB-TMC transport.  All
 * SCPI I/O is serialised through a CommandQueue running on a dedicated
 * worker thread, and the actual command strings are selected via a
 * vendor-configurable ScpiCommandTable.
 *
 * This is a **header-only skeleton** — the .cpp implementation will be
 * written when physical hardware becomes available.  The header is fully
 * documented so that the implementation can be written directly from the
 * Doxygen comments and the protocol specification in
 * `docs/protocols/signal_generator_scpi_spec.md`.
 *
 * @note This header is not currently compiled via CMake.  When the .cpp
 *       implementation is written, both files will be added to the
 *       `if(TARGET Qt6::SerialPort)` conditional block in
 *       `src/hardware/CMakeLists.txt`.
 *
 * @note The GUI always works against SignalGeneratorControllerInterface*.
 *       On platforms without hardware, MockSignalGeneratorController is
 *       used instead.
 *
 * @see SignalGeneratorControllerInterface
 * @see ScpiClient
 * @see CommandQueue
 * @see MockSignalGeneratorController
 * @see docs/protocols/signal_generator_scpi_spec.md
 *
 * @copyright LGPL-3.0-or-later
 */

#pragma once

#include <QMutex>
#include <QString>

#include <cstdint>
#include <memory>

#include "hardware/command_queue.h"
#include "hardware/signal_generator/signal_generator_controller_interface.h"

namespace mwa::hardware {

// Forward declarations — full headers included in the .cpp only.
class ScpiClient;
class ScpiTransport;

/// Default SCPI command timeout in milliseconds.
inline constexpr int kSigGenCommandTimeoutMs = 5000;

/// Timeout for the multi-step connect sequence in milliseconds.
inline constexpr int kSigGenConnectTimeoutMs = 15000;

/// Default serial baud rate for signal generators (bits per second).
inline constexpr qint32 kSigGenDefaultBaudRate = 9600;

/// Default SCPI source channel number (1-based).
inline constexpr int kSigGenDefaultChannel = 1;

/**
 * @enum SignalGeneratorVendor
 * @brief Identifies the signal generator vendor for command-set selection.
 *
 * Different vendors use slightly different SCPI command trees for
 * triangle waveforms, sweep control, and output-state query responses.
 * The vendor is either set explicitly via setVendor() or auto-detected
 * from the `*IDN?` response during connectDevice().
 *
 * @see ScpiCommandTable
 * @see SignalGeneratorController::detectVendorFromIdn()
 */
enum class SignalGeneratorVendor {
  kGeneric,    ///< Generic SCPI (common-subset commands only).
  kRigol,      ///< Rigol DG1000Z / DG800 series.
  kKeysight,   ///< Keysight 33500B / 33600A (formerly Agilent) series.
  kTektronix   ///< Tektronix AFG1022 / AFG3000C series.
};

/**
 * @struct ScpiCommandTable
 * @brief Vendor-specific SCPI command strings for signal generators.
 *
 * Most SCPI commands are identical across Rigol, Keysight, and Tektronix.
 * This table captures the areas that differ between vendors:
 * - Triangle waveform: Keysight uses `FUNC TRI`; Rigol and Tektronix
 *   use `FUNC RAMP` with 50 % symmetry
 * - Sweep enable/disable: Rigol/Tek use `SWE:STAT ON/OFF`; Keysight
 *   uses `FREQ:MODE SWE/CW`
 * - Output query response format: Rigol returns `ON`/`OFF`; Keysight
 *   and Tektronix return `1`/`0`
 *
 * Command strings use Qt `QString::arg()` placeholders: `%1` for the
 * channel number and `%2` for the parameter value.
 *
 * @see scpiCommandsForVendor()
 * @see docs/protocols/signal_generator_scpi_spec.md Section 4
 */
struct ScpiCommandTable {
  QString set_frequency;         ///< e.g. ":SOUR%1:FREQ %2"
  QString query_frequency;       ///< e.g. ":SOUR%1:FREQ?"
  QString set_amplitude;         ///< e.g. ":SOUR%1:VOLT %2"
  QString query_amplitude;       ///< e.g. ":SOUR%1:VOLT?"
  QString set_waveform_sine;     ///< e.g. ":SOUR%1:FUNC SIN"
  QString set_waveform_square;   ///< e.g. ":SOUR%1:FUNC SQU"
  QString set_waveform_triangle; ///< e.g. ":SOUR%1:FUNC TRI" (Keysight) or ":SOUR%1:FUNC RAMP" (Rigol/Tek)
  QString set_triangle_symmetry; ///< e.g. ":SOUR%1:FUNC:RAMP:SYMM 50" (Rigol/Tek only; empty for Keysight)
  QString query_waveform;        ///< e.g. ":SOUR%1:FUNC?"
  QString set_output_on;         ///< e.g. ":OUTP%1 ON"
  QString set_output_off;        ///< e.g. ":OUTP%1 OFF"
  QString query_output;          ///< e.g. ":OUTP%1?"
  QString sweep_enable;          ///< e.g. ":SOUR%1:SWE:STAT ON" (Rigol/Tek) or "SOUR%1:FREQ:MODE SWE" (Keysight)
  QString sweep_disable;         ///< e.g. ":SOUR%1:SWE:STAT OFF" (Rigol/Tek) or "SOUR%1:FREQ:MODE CW" (Keysight)
  QString set_sweep_start;       ///< e.g. ":SOUR%1:FREQ:STAR %2"
  QString set_sweep_stop;        ///< e.g. ":SOUR%1:FREQ:STOP %2"
  QString set_sweep_spacing;     ///< e.g. ":SOUR%1:SWE:SPAC LIN"
  QString set_sweep_time;        ///< e.g. ":SOUR%1:SWE:TIME %2"
  bool output_response_numeric;  ///< @c true if OUTP? returns "1"/"0" (Keysight/Tek); @c false if "ON"/"OFF" (Rigol).
};

/**
 * @brief Return the SCPI command table for the given vendor.
 *
 * Returns a pre-populated ScpiCommandTable with the command strings
 * appropriate for @p vendor.  For kGeneric, the Rigol command set is
 * used as it is the most widely compatible.
 *
 * @param vendor The target instrument vendor.
 * @return A fully populated ScpiCommandTable.
 *
 * @note Defined in signal_generator_controller.cpp (not yet implemented).
 */
[[nodiscard]] ScpiCommandTable scpiCommandsForVendor(
    SignalGeneratorVendor vendor);

/**
 * @class SignalGeneratorController
 * @brief Hardware signal generator controller using SCPI over serial.
 *
 * Implements SignalGeneratorControllerInterface for real SCPI-based signal
 * generators.  All SCPI commands are sent through a ScpiClient instance
 * and serialised via an internal CommandQueue so that only one command
 * is in flight at a time.
 *
 * The driver supports three vendor families — Rigol, Keysight, and
 * Tektronix — with automatic vendor detection from the `*IDN?` identity
 * string.  Vendor-specific command differences (triangle waveform, sweep
 * control, output-state parsing) are captured in a ScpiCommandTable that
 * is selected at connect time.
 *
 * ### Typical usage
 * @code
 *   auto sig_gen = std::make_unique<SignalGeneratorController>();
 *   sig_gen->setPortName("/dev/ttyUSB0");
 *   sig_gen->setBaudRate(115200);
 *   sig_gen->connectDevice();  // async — auto-detects vendor via *IDN?
 * @endcode
 *
 * ### Connection workflow (enqueued on CommandQueue)
 * 1. Create SerialTransport with port_name_ and baud_rate_
 * 2. Create ScpiClient wrapping the transport
 * 3. `scpi_client_->open(port_name_)` — open the serial port
 * 4. `scpi_client_->identify()` — send `*IDN?`, receive identity string
 * 5. Auto-detect vendor from identity (or use pre-set vendor)
 * 6. Load vendor-specific ScpiCommandTable
 * 7. `scpi_client_->reset()` — send `*RST` to reset instrument state
 * 8. `scpi_client_->command("*CLS")` — clear error queue
 * 9. Send `:OUTPn OFF` — ensure output disabled (safety)
 * 10. Query current frequency, amplitude, waveform, output state
 * 11. Transition to DeviceState::kConnected
 *
 * ### Disconnection workflow
 * 1. Send `:OUTPn OFF` — disable output (safety)
 * 2. `scpi_client_->close()` — close serial transport
 * 3. Transition to DeviceState::kDisconnected
 *
 * @note Non-copyable, non-movable.
 *
 * @see SignalGeneratorControllerInterface
 * @see ScpiClient
 * @see CommandQueue
 * @see ScpiCommandTable
 * @see docs/protocols/signal_generator_scpi_spec.md
 */
class SignalGeneratorController
    : public SignalGeneratorControllerInterface {
  Q_OBJECT

 public:
  /**
   * @brief Construct a SignalGeneratorController.
   *
   * The controller starts in DeviceState::kDisconnected.  Call
   * setPortName() and optionally setBaudRate() / setVendor() /
   * setChannel() before connectDevice().
   *
   * @param parent Optional QObject parent for Qt ownership management.
   */
  explicit SignalGeneratorController(QObject* parent = nullptr);

  /**
   * @brief Destructor — disconnects the device and shuts down the
   *        command queue.
   */
  ~SignalGeneratorController() override;

  // Non-copyable, non-movable.
  SignalGeneratorController(const SignalGeneratorController&) = delete;
  SignalGeneratorController& operator=(
      const SignalGeneratorController&) = delete;
  SignalGeneratorController(SignalGeneratorController&&) = delete;
  SignalGeneratorController& operator=(
      SignalGeneratorController&&) = delete;

  // ── Configuration (call before connectDevice) ───────────────

  /**
   * @brief Set the serial port name before connecting.
   *
   * Must be called before connectDevice().  Has no effect while the
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
   * Must be called before connectDevice().  The default is
   * @c kSigGenDefaultBaudRate (9600).
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

  /**
   * @brief Set the instrument vendor explicitly.
   *
   * Overrides automatic vendor detection from `*IDN?`.  If not called,
   * the vendor is auto-detected during connectDevice().  Can be called
   * before connect to force a specific command set, or after connect
   * to override the auto-detected vendor.
   *
   * @param vendor The target instrument vendor.
   *
   * @see SignalGeneratorVendor
   */
  void setVendor(SignalGeneratorVendor vendor);

  /**
   * @brief Return the current instrument vendor.
   *
   * Before connectDevice() this returns the explicitly set vendor (or
   * kGeneric if not set).  After connect it returns the auto-detected
   * or explicitly overridden vendor.
   *
   * @return The current SignalGeneratorVendor value.
   */
  [[nodiscard]] SignalGeneratorVendor vendor() const;

  /**
   * @brief Set the SCPI source channel number.
   *
   * Multi-channel instruments (e.g. Rigol DG1062Z, Keysight 33512B)
   * address outputs as `SOURce1:`, `SOURce2:`, etc.  The default is
   * @c kSigGenDefaultChannel (1).
   *
   * @param channel 1-based channel number.
   */
  void setChannel(int channel);

  /**
   * @brief Return the currently configured SCPI channel number.
   *
   * @return The 1-based channel number.
   */
  [[nodiscard]] int channel() const;

  // ── DeviceInterface overrides ───────────────────────────────

  /**
   * @brief Open the serial port and initialise the instrument.
   *
   * Transitions immediately to DeviceState::kConnecting, then enqueues
   * the full connection sequence on the CommandQueue:
   *
   * 1. Create a SerialTransport with port_name_ and baud_rate_
   * 2. Construct a ScpiClient wrapping the transport
   * 3. `scpi_client_->open(port_name_)` — open the serial port
   * 4. `scpi_client_->identify()` — send `*IDN?`
   * 5. Auto-detect vendor from the IDN response string
   *    (or use the vendor set via setVendor())
   * 6. Load the vendor-specific ScpiCommandTable via
   *    scpiCommandsForVendor()
   * 7. `scpi_client_->reset()` — send `*RST`
   * 8. `scpi_client_->command("*CLS")` — clear status/error queue
   * 9. Send `:OUTPn OFF` — ensure output disabled for safety
   * 10. Query current frequency (`SOURce:FREQ?`), amplitude
   *     (`SOURce:VOLT?`), waveform (`SOURce:FUNC?`), and output
   *     state (`OUTPn?`) — cache results
   * 11. Transition to kConnected and emit stateChanged()
   *
   * If any step fails, the state transitions to kError and
   * errorOccurred() is emitted with a descriptive message.
   *
   * @pre setPortName() must have been called.
   */
  void connectDevice() override;

  /**
   * @brief Disable output and close the serial connection.
   *
   * Enqueues the shutdown sequence on the CommandQueue:
   * 1. Send `:OUTPn OFF` — disable signal output for safety
   * 2. `scpi_client_->close()` — close the serial transport
   * 3. Transition to kDisconnected and emit stateChanged()
   */
  void disconnectDevice() override;

  /**
   * @brief Return the display name for this device.
   *
   * @return A string in the form "Signal Generator (<port_name>)".
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

  // ── SignalGeneratorControllerInterface overrides ─────────────

  /**
   * @brief Set the output frequency via SCPI.
   *
   * Enqueues on the CommandQueue:
   * 1. Send the `set_frequency` command from the vendor command table
   *    (e.g. `:SOUR1:FREQ 1000000` for 1 MHz)
   * 2. Send `SYST:ERR?` to check for instrument errors
   * 3. Query back with `query_frequency` (e.g. `:SOUR1:FREQ?`)
   * 4. Parse the response, cache the value, emit frequencyChanged()
   *
   * On error emits errorOccurred() without changing the cached value.
   *
   * @param hz Output frequency in hertz.
   *
   * @pre Device must be connected.
   */
  void setFrequency(double hz) override;

  /**
   * @brief Return the cached output frequency (thread-safe).
   *
   * @return Output frequency in hertz.
   */
  [[nodiscard]] double frequency() const override;

  /**
   * @brief Set the output amplitude via SCPI.
   *
   * Enqueues on the CommandQueue:
   * 1. Send `set_amplitude` (e.g. `:SOUR1:VOLT 2.5`)
   * 2. Check `SYST:ERR?`
   * 3. Query back with `query_amplitude` (e.g. `:SOUR1:VOLT?`)
   * 4. Parse, cache, emit amplitudeChanged()
   *
   * @param volts Peak amplitude in volts (Vpp).
   *
   * @pre Device must be connected.
   */
  void setAmplitude(double volts) override;

  /**
   * @brief Return the cached output amplitude (thread-safe).
   *
   * @return Peak amplitude in volts (Vpp).
   */
  [[nodiscard]] double amplitude() const override;

  /**
   * @brief Set the output waveform shape via SCPI.
   *
   * Enqueues on the CommandQueue:
   * - **kSine:** send `set_waveform_sine` (e.g. `:SOUR1:FUNC SIN`)
   * - **kSquare:** send `set_waveform_square` (e.g. `:SOUR1:FUNC SQU`)
   * - **kTriangle:**
   *   - **Keysight:** send `set_waveform_triangle` (`:SOUR1:FUNC TRI`)
   *   - **Rigol / Tektronix:** send `set_waveform_triangle`
   *     (`:SOUR1:FUNC RAMP`) followed by `set_triangle_symmetry`
   *     (`:SOUR1:FUNC:RAMP:SYMM 50`) to produce a symmetric triangle
   *
   * Then queries back with `query_waveform`, parses the response via
   * parseWaveformResponse(), caches the value, and emits
   * waveformChanged().
   *
   * @param waveform The desired Waveform value.
   *
   * @pre Device must be connected.
   */
  void setWaveform(Waveform waveform) override;

  /**
   * @brief Return the cached output waveform shape (thread-safe).
   *
   * @return The current Waveform value.
   */
  [[nodiscard]] Waveform waveform() const override;

  /**
   * @brief Enable or disable the signal output via SCPI.
   *
   * Enqueues on the CommandQueue:
   * 1. Send `set_output_on` or `set_output_off` depending on
   *    @p enabled (e.g. `:OUTP1 ON` or `:OUTP1 OFF`)
   * 2. Query back with `query_output` (e.g. `:OUTP1?`)
   * 3. Parse response via parseOutputResponse() — handles both
   *    numeric (`1`/`0`) and text (`ON`/`OFF`) formats
   * 4. Cache the value, emit outputStateChanged()
   *
   * @param enabled @c true to enable the output, @c false to disable.
   *
   * @pre Device must be connected.
   */
  void setOutputEnabled(bool enabled) override;

  /**
   * @brief Return the cached output enable state (thread-safe).
   *
   * @return @c true if the output is currently enabled.
   */
  [[nodiscard]] bool isOutputEnabled() const override;

  /**
   * @brief Configure a frequency sweep via SCPI.
   *
   * Enqueues the sweep configuration sequence on the CommandQueue:
   * 1. Send `sweep_disable` — disable sweep while reconfiguring
   * 2. Send `set_sweep_start` with @p start_hz
   * 3. Send `set_sweep_stop` with @p stop_hz
   * 4. Send `set_sweep_spacing` (always linear)
   * 5. Compute sweep time from the step size:
   *    `time_s = (stop_hz - start_hz) / step_hz * dwell_per_step`
   * 6. Send `set_sweep_time` with the computed time
   * 7. Send `sweep_enable` — activate the sweep
   *
   * No query-back is performed for sweep parameters.
   *
   * @param start_hz Start frequency of the sweep in hertz.
   * @param stop_hz  Stop frequency of the sweep in hertz.
   * @param step_hz  Frequency step size in hertz.  Used to compute the
   *                 total sweep time (smaller steps → longer sweep).
   *
   * @pre Device must be connected.
   */
  void configureSweep(
      double start_hz, double stop_hz, double step_hz) override;

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
   * @brief Auto-detect the instrument vendor from an `*IDN?` response.
   *
   * Parses the identity string (format:
   * `<Manufacturer>,<Model>,<Serial>,<Firmware>`) and checks the
   * manufacturer field for:
   * - "RIGOL" → kRigol
   * - "Keysight" or "Agilent" → kKeysight
   * - "TEKTRONIX" → kTektronix
   *
   * Falls back to kGeneric if no known vendor is detected.
   *
   * @param idn_response The raw `*IDN?` response string.
   * @return The detected SignalGeneratorVendor.
   */
  [[nodiscard]] SignalGeneratorVendor detectVendorFromIdn(
      const QString& idn_response) const;

  /**
   * @brief Parse a `SOURce:FUNC?` response into a Waveform enum.
   *
   * Maps common response strings to Waveform values:
   * - "SIN" or "SINUSOID" → kSine
   * - "SQU" or "SQUARE" → kSquare
   * - "TRI", "TRIANGLE", "RAMP" → kTriangle
   *
   * @param response The trimmed SCPI response string.
   * @return The corresponding Waveform value, or kSine as fallback.
   */
  [[nodiscard]] Waveform parseWaveformResponse(
      const QString& response) const;

  /**
   * @brief Parse an `OUTPut?` response into a boolean.
   *
   * Handles both response formats:
   * - Numeric: "1" → true, "0" → false (Keysight, Tektronix)
   * - Text: "ON" → true, "OFF" → false (Rigol)
   *
   * @param response The trimmed SCPI response string.
   * @return @c true if the output is enabled.
   */
  [[nodiscard]] bool parseOutputResponse(
      const QString& response) const;

  /// Serialises all SCPI commands onto a single worker thread.
  std::unique_ptr<CommandQueue> command_queue_;

  /// SCPI command/query interface.  Created during connectDevice().
  std::unique_ptr<ScpiClient> scpi_client_;

  mutable QMutex mutex_;  ///< Guards all cached state below.

  // ── Cached state (read/written under mutex_) ─────────────────

  DeviceState state_{DeviceState::kDisconnected};  ///< Connection state.
  double frequency_{0.0};                          ///< Cached frequency in Hz.
  double amplitude_{0.0};                          ///< Cached amplitude in Vpp.
  Waveform waveform_{Waveform::kSine};             ///< Cached waveform shape.
  bool output_enabled_{false};                     ///< Cached output state.

  // ── Configuration (set before connectDevice) ──────────────────

  QString port_name_;           ///< Serial port name (e.g. "/dev/ttyUSB0").
  qint32 baud_rate_{kSigGenDefaultBaudRate};  ///< Serial baud rate (bps).
  SignalGeneratorVendor vendor_{SignalGeneratorVendor::kGeneric};  ///< Instrument vendor.
  int channel_{kSigGenDefaultChannel};  ///< SCPI source channel (1-based).
  ScpiCommandTable commands_;   ///< Active vendor-specific command table.
};

}  // namespace mwa::hardware

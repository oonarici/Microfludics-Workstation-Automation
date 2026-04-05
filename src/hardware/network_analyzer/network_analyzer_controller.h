/**
 * @file network_analyzer_controller.h
 * @brief Hardware VNA driver skeleton for SCPI-based network analyzers.
 * @author onuronarici
 * @date 2026-04-05
 *
 * Declares the NetworkAnalyzerController class, which drives SCPI-based
 * vector network analyzers (Keysight ENA E5063A/E5080A, Rohde & Schwarz
 * ZNB series, and compatible models) over USB-TMC, LAN (TCP port 5025),
 * or serial transports.  All SCPI I/O is serialised through a CommandQueue
 * running on a dedicated worker thread, and vendor-specific command
 * differences are captured in a VnaScpiCommandTable selected at connect
 * time.
 *
 * This is a **header-only skeleton** -- the .cpp implementation will be
 * written when physical hardware becomes available.  The header is fully
 * documented so that the implementation can be written directly from the
 * Doxygen comments and the protocol specification in
 * `docs/protocols/vna_scpi_spec.md`.
 *
 * @note This header is not currently compiled via CMake.  When the .cpp
 *       implementation is written, both files will be added to the
 *       `if(TARGET Qt6::SerialPort)` conditional block in
 *       `src/hardware/CMakeLists.txt`.
 *
 * @note The GUI always works against NetworkAnalyzerControllerInterface*.
 *       On platforms without hardware, MockNetworkAnalyzerController is
 *       used instead.
 *
 * @see NetworkAnalyzerControllerInterface
 * @see ScpiClient
 * @see CommandQueue
 * @see MockNetworkAnalyzerController
 * @see docs/protocols/vna_scpi_spec.md
 *
 * @copyright LGPL-3.0-or-later
 */

#pragma once

#include <QMutex>
#include <QString>
#include <QVector>

#include <cstdint>
#include <memory>

#include "hardware/command_queue.h"
#include "hardware/network_analyzer/network_analyzer_controller_interface.h"

namespace mwa::hardware {

// Forward declarations -- full headers included in the .cpp only.
class ScpiClient;
class ScpiTransport;

/// Default SCPI command timeout in milliseconds.
inline constexpr int kVnaCommandTimeoutMs = 10000;

/// Timeout for the multi-step connect sequence in milliseconds.
inline constexpr int kVnaConnectTimeoutMs = 20000;

/// Timeout for a single S-parameter sweep in milliseconds.
inline constexpr int kVnaSweepTimeoutMs = 30000;

/// Default number of sweep points.
inline constexpr int kVnaDefaultNumPoints = 201;

/// Default sweep start frequency in hertz (1 MHz).
inline constexpr double kVnaDefaultStartHz = 1.0e6;

/// Default sweep stop frequency in hertz (10 MHz).
inline constexpr double kVnaDefaultStopHz = 10.0e6;

/// Default IF bandwidth in hertz (1 kHz -- good for PZT characterisation).
inline constexpr double kVnaDefaultIfBandwidthHz = 1000.0;

/// Default SCPI channel number (1-based).
inline constexpr int kVnaDefaultChannel = 1;

/**
 * @enum VnaVendor
 * @brief Identifies the VNA vendor for SCPI command-set selection.
 *
 * Keysight ENA and Rohde & Schwarz ZNB use slightly different SCPI
 * commands for S-parameter definition, trace data retrieval, and
 * trigger/wait synchronisation.  The vendor is either set explicitly
 * via setVendor() or auto-detected from the `*IDN?` response during
 * connectDevice().
 *
 * @see VnaScpiCommandTable
 * @see NetworkAnalyzerController::detectVendorFromIdn()
 */
enum class VnaVendor {
  kGeneric,       ///< Generic SCPI (Keysight command set as fallback).
  kKeysight,      ///< Keysight ENA series (E5063A, E5080A, E5071C).
  kRohdeSchwarz   ///< Rohde & Schwarz ZNB/ZVA series.
};

/**
 * @struct VnaScpiCommandTable
 * @brief Vendor-specific SCPI command strings for network analyzers.
 *
 * Most SCPI commands are identical across Keysight and R&S VNAs.
 * This table captures the areas that differ:
 *
 * - **S-parameter definition**: Keysight uses
 *   `CALC:PAR:DEF:EXT "name","S11"` plus an explicit
 *   `DISP:WIND:TRAC:FEED "name"` to assign the trace to a display
 *   window.  R&S uses `CALC:PAR:SDEF "Trc1","S11"` and the trace
 *   display is automatic.
 * - **Formatted data query**: Keysight uses `CALC:SEL:DATA? FDATA`;
 *   R&S uses `CALC:DATA? FDAT`.
 * - **Complex data query**: Keysight uses `CALC:SEL:DATA? SDATA`;
 *   R&S uses `CALC:DATA? SDAT`.
 * - **Trigger synchronisation**: Keysight uses `INIT:IMM; *OPC?`
 *   (returns "1" on completion); R&S uses `INIT:IMM; *WAI` (blocks
 *   until complete).
 *
 * Command strings use Qt `QString::arg()` placeholders: `%1` for
 * the channel number, `%2` and `%3` for parameter values.
 *
 * @see vnaCommandsForVendor()
 * @see docs/protocols/vna_scpi_spec.md Section 6
 */
struct VnaScpiCommandTable {
  // ── S-parameter definition ────────────────────────────────────
  /// Define the S-parameter measurement.
  /// Keysight: `CALC%1:PAR:DEF:EXT "%2","%3"`
  /// R&S:      `CALC%1:PAR:SDEF "%2","%3"`
  QString define_measurement;

  /// Assign trace to display (Keysight only; empty for R&S).
  /// Keysight: `DISP:WIND%1:TRAC1:FEED "%2"`
  QString display_trace_feed;

  // ── Data format & retrieval ───────────────────────────────────
  /// Set data format to magnitude (dB).
  /// Both: `CALC%1:SEL:FORM MLOG`  (identical syntax)
  QString set_format_mlog;

  /// Query formatted trace data (magnitude in dB).
  /// Keysight: `CALC%1:SEL:DATA? FDATA`
  /// R&S:      `CALC%1:DATA? FDAT`
  QString query_fdata;

  /// Query complex S-parameter data (real/imag pairs).
  /// Keysight: `CALC%1:SEL:DATA? SDATA`
  /// R&S:      `CALC%1:DATA? SDAT`
  QString query_sdata;

  // ── Frequency configuration ───────────────────────────────────
  /// Set sweep start frequency.  Both: `SENS%1:FREQ:STAR %2`
  QString set_start_freq;

  /// Query sweep start frequency.  Both: `SENS%1:FREQ:STAR?`
  QString query_start_freq;

  /// Set sweep stop frequency.  Both: `SENS%1:FREQ:STOP %2`
  QString set_stop_freq;

  /// Query sweep stop frequency.  Both: `SENS%1:FREQ:STOP?`
  QString query_stop_freq;

  // ── Sweep configuration ───────────────────────────────────────
  /// Set number of sweep points.  Both: `SENS%1:SWE:POIN %2`
  QString set_sweep_points;

  /// Query number of sweep points.  Both: `SENS%1:SWE:POIN?`
  QString query_sweep_points;

  /// Set IF bandwidth.  Both: `SENS%1:BAND %2`
  QString set_if_bandwidth;

  /// Set sweep type to linear.  Both: `SENS%1:SWE:TYPE LIN`
  QString set_sweep_type_linear;

  // ── Trigger & initiation ──────────────────────────────────────
  /// Set single sweep mode.  Both: `SENS%1:SWE:MODE SING`
  QString set_sweep_mode_single;

  /// Set bus trigger source.
  /// Both: `TRIG:SEQ:SOUR BUS` (identical syntax)
  QString set_trigger_bus;

  /// Initiate a sweep and wait for completion.
  /// Keysight: `INIT%1:IMM; *OPC?`  (returns "1" on completion)
  /// R&S:      `INIT%1:IMM; *WAI`   (blocks until complete)
  QString initiate_and_wait;

  /// Whether initiate_and_wait requires reading a response.
  /// @c true for Keysight (`*OPC?` returns "1"), @c false for R&S
  /// (`*WAI` blocks without response).
  bool initiate_returns_response;
};

/**
 * @brief Return the SCPI command table for the given VNA vendor.
 *
 * Returns a pre-populated VnaScpiCommandTable with the command strings
 * appropriate for @p vendor.  For kGeneric, the Keysight command set
 * is used as it is the most widely compatible.
 *
 * @param vendor The target instrument vendor.
 * @return A fully populated VnaScpiCommandTable.
 *
 * @note Defined in network_analyzer_controller.cpp (not yet
 *       implemented).
 */
[[nodiscard]] VnaScpiCommandTable vnaCommandsForVendor(VnaVendor vendor);

/**
 * @class NetworkAnalyzerController
 * @brief Hardware VNA controller using SCPI over VISA / TCP / serial.
 *
 * Implements NetworkAnalyzerControllerInterface for real SCPI-based
 * vector network analyzers.  All SCPI commands are sent through a
 * ScpiClient instance and serialised via an internal CommandQueue so
 * that only one command is in flight at a time.
 *
 * The driver supports two vendor families -- Keysight ENA and
 * Rohde & Schwarz ZNB -- with automatic vendor detection from the
 * `*IDN?` identity string.  Vendor-specific command differences
 * (S-parameter definition, data queries, trigger/wait) are captured
 * in a VnaScpiCommandTable selected at connect time.
 *
 * ### Typical usage
 * @code
 *   auto vna = std::make_unique<NetworkAnalyzerController>();
 *   vna->setResourceString("TCPIP0::192.168.1.100::5025::SOCKET");
 *   vna->connectDevice();  // async -- auto-detects vendor via *IDN?
 *   // ... after stateChanged(kConnected) ...
 *   vna->setFrequencyRange(1.0e6, 10.0e6);
 *   vna->setNumPoints(201);
 *   vna->measureSParameters();  // async -- emits measurementComplete()
 *   // ... after measurementComplete() ...
 *   auto freqs = vna->traceFrequencies();
 *   auto mags  = vna->traceMagnitudes();
 * @endcode
 *
 * ### Connection workflow (enqueued on CommandQueue)
 * 1. Create the appropriate ScpiTransport for the resource string
 *    (TCP socket, USB-TMC, or serial)
 * 2. Create ScpiClient wrapping the transport
 * 3. `scpi_client_->open(resource_string_)` -- open the transport
 * 4. `scpi_client_->identify()` -- send `*IDN?`, receive identity
 * 5. Auto-detect vendor from identity (or use pre-set vendor)
 * 6. Load vendor-specific VnaScpiCommandTable
 * 7. `scpi_client_->reset()` -- send `*RST` to preset instrument
 * 8. `scpi_client_->command("*CLS")` -- clear status/error queue
 * 9. Configure single-sweep mode and bus trigger source
 * 10. Define S11 measurement on channel 1
 * 11. Set default data format to magnitude (dB)
 * 12. Set default frequency range, points, and IF bandwidth
 * 13. Transition to DeviceState::kConnected
 *
 * ### Measurement workflow (enqueued on CommandQueue)
 * 1. Emit measurementStarted()
 * 2. Set is_measuring_ flag to @c true
 * 3. Send initiate-and-wait command (vendor-specific)
 * 4. Query formatted trace data via the `query_fdata` command
 * 5. Parse comma-separated ASCII response into magnitude vector
 * 6. Compute frequency vector from start/stop/points
 * 7. Cache both vectors, clear is_measuring_ flag
 * 8. Emit measurementComplete()
 *
 * ### Disconnection workflow
 * 1. `scpi_client_->close()` -- close the transport
 * 2. Transition to DeviceState::kDisconnected
 *
 * @note Non-copyable, non-movable.
 *
 * @see NetworkAnalyzerControllerInterface
 * @see ScpiClient
 * @see CommandQueue
 * @see VnaScpiCommandTable
 * @see docs/protocols/vna_scpi_spec.md
 */
class NetworkAnalyzerController
    : public NetworkAnalyzerControllerInterface {
  Q_OBJECT

 public:
  /**
   * @brief Construct a NetworkAnalyzerController.
   *
   * The controller starts in DeviceState::kDisconnected with default
   * sweep parameters (1--10 MHz, 201 points, 1 kHz IF bandwidth).
   * Call setResourceString() and optionally setVendor() / setChannel()
   * before connectDevice().
   *
   * @param parent Optional QObject parent for Qt ownership management.
   */
  explicit NetworkAnalyzerController(QObject* parent = nullptr);

  /**
   * @brief Destructor -- disconnects the device and shuts down the
   *        command queue.
   */
  ~NetworkAnalyzerController() override;

  // Non-copyable, non-movable.
  NetworkAnalyzerController(const NetworkAnalyzerController&) = delete;
  NetworkAnalyzerController& operator=(
      const NetworkAnalyzerController&) = delete;
  NetworkAnalyzerController(NetworkAnalyzerController&&) = delete;
  NetworkAnalyzerController& operator=(
      NetworkAnalyzerController&&) = delete;

  // ── Configuration (call before connectDevice) ───────────────

  /**
   * @brief Set the VISA resource string or TCP address before
   *        connecting.
   *
   * Accepted formats:
   * - VISA USB-TMC: `"USB0::0x0957::0x0D09::MY12345678::INSTR"`
   * - TCP/LXI:      `"TCPIP0::192.168.1.100::5025::SOCKET"`
   * - GPIB:         `"GPIB0::16::INSTR"`
   * - Serial:       `"/dev/ttyUSB0"` or `"COM3"`
   *
   * Must be called before connectDevice().  Has no effect while
   * connected.
   *
   * @warning Not thread-safe -- call only while disconnected.
   * @param resource The VISA resource string or serial port name.
   */
  void setResourceString(const QString& resource);

  /**
   * @brief Return the currently configured resource string.
   *
   * @return The resource string.
   */
  [[nodiscard]] QString resourceString() const;

  /**
   * @brief Set the instrument vendor explicitly.
   *
   * Overrides automatic vendor detection from `*IDN?`.  If not
   * called, the vendor is auto-detected during connectDevice().
   *
   * @warning Not thread-safe -- call only while disconnected.
   * @param vendor The target instrument vendor.
   *
   * @see VnaVendor
   */
  void setVendor(VnaVendor vendor);

  /**
   * @brief Return the current instrument vendor.
   *
   * Before connectDevice() this returns the explicitly set vendor
   * (or kGeneric if not set).  After connect it returns the
   * auto-detected or explicitly overridden vendor.
   *
   * @return The current VnaVendor value.
   */
  [[nodiscard]] VnaVendor vendor() const;

  /**
   * @brief Set the SCPI channel number.
   *
   * Multi-channel VNAs address measurements as `SENSe1:`, `SENSe2:`,
   * etc.  The default is @c kVnaDefaultChannel (1).
   *
   * @warning Not thread-safe -- call only while disconnected.
   * @param channel 1-based channel number.
   */
  void setChannel(int channel);

  /**
   * @brief Return the currently configured SCPI channel number.
   *
   * @return The 1-based channel number.
   */
  [[nodiscard]] int channel() const;

  /**
   * @brief Set the IF (intermediate frequency) bandwidth.
   *
   * The IF bandwidth controls the measurement noise floor vs. sweep
   * speed trade-off.  Smaller bandwidths give lower noise at the
   * cost of longer sweep times.  The default is
   * @c kVnaDefaultIfBandwidthHz (1 kHz).
   *
   * If the device is connected, the new value is sent to the
   * instrument via `SENS:BAND` immediately.  Otherwise it is
   * cached and applied during connectDevice().
   *
   * @param hz IF bandwidth in hertz (e.g. 100, 1000, 10000).
   */
  void setIfBandwidth(double hz);

  /**
   * @brief Return the currently configured IF bandwidth.
   *
   * @return IF bandwidth in hertz.
   */
  [[nodiscard]] double ifBandwidth() const;

  // ── DeviceInterface overrides ───────────────────────────────

  /**
   * @brief Open the transport and initialise the VNA.
   *
   * Transitions immediately to DeviceState::kConnecting, then
   * enqueues the full connection sequence on the CommandQueue:
   *
   * 1. Create the appropriate ScpiTransport for resource_string_
   * 2. Construct a ScpiClient wrapping the transport
   * 3. `scpi_client_->open(resource_string_)` -- open transport
   * 4. `scpi_client_->identify()` -- send `*IDN?`
   * 5. Auto-detect vendor from the IDN response
   *    (or use the vendor set via setVendor())
   * 6. Load the vendor-specific VnaScpiCommandTable
   * 7. `scpi_client_->reset()` -- send `*RST`
   * 8. `scpi_client_->command("*CLS")` -- clear status/errors
   * 9. Set single-sweep mode:
   *    `SENS<ch>:SWE:MODE SING`
   * 10. Set bus trigger source: `TRIG:SEQ:SOUR BUS`
   * 11. Define S11 measurement on the channel using
   *     vendor-specific define_measurement command
   * 12. Set data format to magnitude (dB) via set_format_mlog
   * 13. Set start/stop frequencies, number of points, and
   *     IF bandwidth to the cached configuration values
   * 14. Transition to kConnected and emit stateChanged()
   *
   * If any step fails, the state transitions to kError and
   * errorOccurred() is emitted with a descriptive message.
   *
   * @pre setResourceString() must have been called.
   */
  void connectDevice() override;

  /**
   * @brief Close the transport connection.
   *
   * Enqueues the shutdown sequence on the CommandQueue:
   * 1. `scpi_client_->close()` -- close the transport
   * 2. Transition to kDisconnected and emit stateChanged()
   */
  void disconnectDevice() override;

  /**
   * @brief Return the display name for this device.
   *
   * @return A string in the form
   *         "Network Analyzer (<resource_string>)".
   */
  [[nodiscard]] QString deviceName() const override;

  /**
   * @brief Return the current connection state (thread-safe).
   *
   * @return The current DeviceState value, read under a mutex.
   */
  [[nodiscard]] DeviceState state() const override;

  /**
   * @brief Query whether the device is fully connected
   *        (thread-safe).
   *
   * @return @c true if the device state is DeviceState::kConnected.
   */
  [[nodiscard]] bool isConnected() const override;

  // ── NetworkAnalyzerControllerInterface overrides ────────────

  /**
   * @brief Set the frequency sweep range via SCPI.
   *
   * Enqueues on the CommandQueue:
   * 1. Send `set_start_freq` (e.g. `SENS1:FREQ:STAR 1000000`)
   * 2. Send `set_stop_freq` (e.g. `SENS1:FREQ:STOP 10000000`)
   * 3. Send `SYST:ERR?` to check for instrument errors
   * 4. Query back with `query_start_freq` and `query_stop_freq`
   * 5. Parse responses, cache values, emit frequencyRangeChanged()
   *
   * On error emits errorOccurred() without changing cached values.
   *
   * @param start_hz Start frequency in hertz.
   * @param stop_hz  Stop frequency in hertz.
   *
   * @pre Device must be connected.
   */
  void setFrequencyRange(double start_hz, double stop_hz) override;

  /**
   * @brief Return the cached sweep start frequency (thread-safe).
   *
   * @return Start frequency in hertz.
   */
  [[nodiscard]] double startFrequency() const override;

  /**
   * @brief Return the cached sweep stop frequency (thread-safe).
   *
   * @return Stop frequency in hertz.
   */
  [[nodiscard]] double stopFrequency() const override;

  /**
   * @brief Set the number of sweep points via SCPI.
   *
   * Enqueues on the CommandQueue:
   * 1. Send `set_sweep_points` (e.g. `SENS1:SWE:POIN 201`)
   * 2. Send `SYST:ERR?` to check for errors
   * 3. Query back with `query_sweep_points`
   * 4. Parse response, cache value, emit numPointsChanged()
   *
   * @param points Number of frequency points (typically 51--1001).
   *
   * @pre Device must be connected.
   */
  void setNumPoints(int points) override;

  /**
   * @brief Return the cached number of sweep points (thread-safe).
   *
   * @return Number of frequency points.
   */
  [[nodiscard]] int numPoints() const override;

  /**
   * @brief Trigger an asynchronous S-parameter measurement.
   *
   * Enqueues the measurement sequence on the CommandQueue:
   * 1. Set is_measuring_ to @c true
   * 2. Emit measurementStarted()
   * 3. Send the initiate-and-wait command from the vendor table:
   *    - **Keysight:** `INIT1:IMM; *OPC?` -- read "1" response
   *    - **R&S:** `INIT1:IMM; *WAI` -- blocks until sweep done
   * 4. Send `query_fdata` to retrieve formatted trace data
   *    (comma-separated ASCII magnitude values in dB)
   * 5. Parse response: split on commas, convert each token to
   *    @c double via `QString::toDouble()`
   * 6. Compute frequency vector:
   *    `f[i] = start_hz + i * (stop_hz - start_hz) / (n - 1)`
   * 7. Cache both vectors under mutex_
   * 8. Send `SYST:ERR?` to verify no instrument errors
   * 9. Set is_measuring_ to @c false
   * 10. Emit measurementComplete()
   *
   * If any step fails, is_measuring_ is cleared and
   * errorOccurred() is emitted.
   *
   * @pre Device must be connected.  A previous measurement must
   *      not be in progress (isMeasuring() == false).
   */
  void measureSParameters() override;

  /**
   * @brief Return the frequency axis of the last measurement
   *        (thread-safe).
   *
   * The vector is computed (not queried from the instrument)
   * using the cached start/stop frequencies and number of points
   * at the time the measurement was triggered.
   *
   * @return Vector of frequency values in hertz, or empty if no
   *         measurement has been performed.
   */
  [[nodiscard]] QVector<double> traceFrequencies() const override;

  /**
   * @brief Return the magnitude trace of the last measurement
   *        (thread-safe).
   *
   * Contains the parsed FDATA response -- S11 magnitudes in dB
   * at each frequency point.
   *
   * @return Vector of S-parameter magnitudes in dB, or empty if
   *         no measurement has been performed.
   */
  [[nodiscard]] QVector<double> traceMagnitudes() const override;

  /**
   * @brief Query whether a measurement is in progress
   *        (thread-safe).
   *
   * @return @c true while a sweep is running on the CommandQueue.
   */
  [[nodiscard]] bool isMeasuring() const override;

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
   * @brief Auto-detect the VNA vendor from an `*IDN?` response.
   *
   * Parses the identity string (format:
   * `<Manufacturer>,<Model>,<Serial>,<Firmware>`) and checks the
   * manufacturer field for:
   * - "Keysight" or "Agilent" --> kKeysight
   * - "Rohde" or "R&S" --> kRohdeSchwarz
   *
   * Falls back to kGeneric if no known vendor is detected.
   *
   * @param idn_response The raw `*IDN?` response string.
   * @return The detected VnaVendor.
   */
  [[nodiscard]] VnaVendor detectVendorFromIdn(
      const QString& idn_response) const;

  /**
   * @brief Parse a comma-separated FDATA response into a vector
   *        of magnitudes.
   *
   * Splits the response on commas, trims whitespace, and converts
   * each token to @c double.  Returns an empty vector on parse
   * failure.
   *
   * @param response The raw ASCII FDATA response from the VNA.
   * @param expected_points The expected number of data points.
   * @return Vector of magnitude values in dB.
   */
  [[nodiscard]] QVector<double> parseFdataResponse(
      const QString& response, int expected_points) const;

  /**
   * @brief Compute the linear frequency vector from the sweep
   *        parameters.
   *
   * Produces @p num_points evenly-spaced frequency values from
   * @p start_hz to @p stop_hz inclusive.
   *
   * @param start_hz Start frequency in hertz.
   * @param stop_hz  Stop frequency in hertz.
   * @param num_points Number of sweep points.
   * @return Vector of frequency values in hertz.
   */
  [[nodiscard]] static QVector<double> computeFrequencyVector(
      double start_hz, double stop_hz, int num_points);

  /// Serialises all SCPI commands onto a single worker thread.
  std::unique_ptr<CommandQueue> command_queue_;

  /// SCPI command/query interface.  Created during connectDevice().
  std::unique_ptr<ScpiClient> scpi_client_;

  mutable QMutex mutex_;  ///< Guards all cached state below.

  // ── Cached state (read/written under mutex_) ─────────────────

  DeviceState state_{DeviceState::kDisconnected};  ///< Connection state.
  double start_hz_{kVnaDefaultStartHz};   ///< Sweep start freq (Hz).
  double stop_hz_{kVnaDefaultStopHz};     ///< Sweep stop freq (Hz).
  int num_points_{kVnaDefaultNumPoints};  ///< Number of sweep points.
  bool is_measuring_{false};              ///< Sweep in progress flag.
  QVector<double> trace_frequencies_;     ///< Last measurement freqs.
  QVector<double> trace_magnitudes_;      ///< Last measurement mags.

  // ── Configuration (set before connectDevice) ──────────────────

  /// VISA resource string or serial port name.
  QString resource_string_;
  VnaVendor vendor_{VnaVendor::kGeneric};      ///< Instrument vendor.
  int channel_{kVnaDefaultChannel};            ///< SCPI channel (1-based).
  double if_bandwidth_hz_{kVnaDefaultIfBandwidthHz};  ///< IF BW (Hz).
  VnaScpiCommandTable commands_;  ///< Active vendor command table.
};

}  // namespace mwa::hardware

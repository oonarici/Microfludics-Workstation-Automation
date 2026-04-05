/**
 * @file experiment_session.h
 * @brief Experiment session data model for MWA.
 * @author MWA Team
 * @date 2026-04-05
 *
 * Defines the ExperimentSession class and its supporting data structs.
 * An ExperimentSession accumulates hardware telemetry captured during a
 * live experiment run. All stored types are Qt types — no vendor SDK types
 * may appear here (see docs/hardware_data_flow.md, Interface Type Contract).
 *
 * @copyright LGPL-3.0-or-later
 */

#pragma once

#include <QDateTime>
#include <QImage>
#include <QObject>
#include <QString>
#include <QVector>

namespace mwa::analysis {

// ---------------------------------------------------------------------------
// Per-device data structs
// ---------------------------------------------------------------------------

/**
 * @struct CameraFrame
 * @brief One captured camera frame with its acquisition timestamp.
 */
struct CameraFrame {
  QImage    image;      ///< Captured image (Qt type — never a Pylon grab result).
  QDateTime timestamp;  ///< Wall-clock time at the moment of capture.
};

/**
 * @struct VnaMeasurement
 * @brief One complete VNA frequency sweep with its results.
 *
 * Frequencies and magnitudes are parallel arrays: @c frequencies[i] is the
 * excitation frequency in Hz and @c magnitudes[i] is the S-parameter
 * magnitude at that frequency in dB.
 */
struct VnaMeasurement {
  double          start_frequency;  ///< Sweep start frequency (Hz).
  double          stop_frequency;   ///< Sweep stop frequency (Hz).
  QVector<double> frequencies;      ///< Per-point frequency values (Hz).
  QVector<double> magnitudes;       ///< Per-point S-parameter magnitudes (dB).
  QDateTime       timestamp;        ///< Wall-clock time when the sweep completed.
};

/**
 * @struct PumpSample
 * @brief One syringe-pump telemetry sample.
 */
struct PumpSample {
  double    position;   ///< Cumulative volume dispensed (µL).
  double    flow_rate;  ///< Instantaneous flow rate at sample time (µL/min).
  QDateTime timestamp;  ///< Wall-clock time of the sample.
};

/**
 * @struct LedSample
 * @brief One LED state sample.
 */
struct LedSample {
  bool      power_on;  ///< true if the LED output is active.
  double    intensity; ///< Brightness level (0.0–100.0 %).
  QDateTime timestamp; ///< Wall-clock time of the sample.
};

/**
 * @struct SigGenSample
 * @brief One signal-generator parameter sample.
 */
struct SigGenSample {
  double    frequency;  ///< Output frequency (Hz).
  double    amplitude;  ///< Output amplitude (Vpp).
  QDateTime timestamp;  ///< Wall-clock time of the sample.
};

/**
 * @struct StageSample
 * @brief One XYZ stage position sample.
 */
struct StageSample {
  double    x;          ///< X-axis position (mm).
  double    y;          ///< Y-axis position (mm).
  double    z;          ///< Z-axis position (mm).
  QDateTime timestamp;  ///< Wall-clock time of the sample.
};

// ---------------------------------------------------------------------------
// ExperimentSession
// ---------------------------------------------------------------------------

/**
 * @class ExperimentSession
 * @brief Accumulates hardware telemetry for one experiment run.
 *
 * An ExperimentSession has a simple lifecycle: it starts with start(),
 * accumulates data via add*() slots, and ends with end(). While active,
 * data from any hardware controller can be fed directly from its signals
 * into the corresponding add*() slot.
 *
 * All stored types are Qt primitives or Qt classes (QImage, QVector<double>,
 * QDateTime, etc.). No vendor SDK types (Pylon, QmixSDK, SCPI objects) may
 * appear here — they must be converted at the driver boundary before being
 * passed to this class.
 *
 * @note The class is @b not thread-safe. All calls to add*() and lifecycle
 *       methods must be made from the same thread (typically the GUI thread
 *       via Qt queued connections from hardware worker threads).
 *
 * @see CameraFrame
 * @see VnaMeasurement
 * @see PumpSample
 * @see LedSample
 * @see SigGenSample
 * @see StageSample
 */
class ExperimentSession : public QObject {
  Q_OBJECT

 public:
  /**
   * @brief Construct an idle session.
   *
   * @param parent Optional QObject parent for Qt ownership management.
   */
  explicit ExperimentSession(QObject* parent = nullptr);

  // -------------------------------------------------------------------------
  // Session lifecycle
  // -------------------------------------------------------------------------

  /**
   * @brief Start a new session.
   *
   * Records the current wall-clock time as startTime(), clears any
   * previously stored data, and sets the session to active. Emits
   * sessionStarted().
   *
   * @param name        Human-readable session name (e.g. "Run 42").
   * @param description Optional free-text description.
   *
   * @note Calling start() on an already-active session ends it first
   *       (emits sessionEnded()), then starts the new one.
   */
  void start(const QString& name, const QString& description = {});

  /**
   * @brief End the active session.
   *
   * Records the current wall-clock time as endTime() and marks the session
   * as inactive. Emits sessionEnded(). No-op if the session is already idle.
   */
  void end();

  /**
   * @brief Return true while data is being accumulated.
   *
   * @return true if start() has been called and end() has not yet been called.
   */
  [[nodiscard]] bool isActive() const;

  // -------------------------------------------------------------------------
  // Session metadata
  // -------------------------------------------------------------------------

  /**
   * @brief Return the session name set by start().
   *
   * @return Session name, or an empty string if the session has never started.
   */
  [[nodiscard]] QString name() const;

  /**
   * @brief Return the optional description set by start().
   *
   * @return Session description, or an empty string if not provided.
   */
  [[nodiscard]] QString description() const;

  /**
   * @brief Return the wall-clock time when start() was last called.
   *
   * @return Start timestamp, or a null QDateTime if the session has never
   *         started.
   */
  [[nodiscard]] QDateTime startTime() const;

  /**
   * @brief Return the wall-clock time when end() was last called.
   *
   * @return End timestamp, or a null QDateTime if the session is still active
   *         or has never ended.
   */
  [[nodiscard]] QDateTime endTime() const;

  // -------------------------------------------------------------------------
  // Data ingestion — connect hardware controller signals to these slots
  // -------------------------------------------------------------------------

  /**
   * @brief Append a camera frame.
   *
   * Timestamps the frame at the moment of the call and appends it to the
   * internal frame list. Emits cameraFrameAdded() with the new total count.
   *
   * @param image The captured frame (Qt Grayscale8 or RGB32 image).
   *
   * @note Silently ignored if the session is not active.
   */
  void addCameraFrame(const QImage& image);

  /**
   * @brief Append a VNA measurement.
   *
   * Records a completed frequency sweep. @p frequencies and @p magnitudes
   * must each have exactly @p num_points elements; if any size disagrees the
   * call is a no-op and a warning is logged.
   *
   * @param start_frequency  Sweep start frequency (Hz).
   * @param stop_frequency   Sweep stop frequency (Hz).
   * @param num_points       Expected number of sweep points (validation only —
   *                         not stored; use frequencies.size() on the returned
   *                         VnaMeasurement to query the count).
   * @param frequencies      Per-point excitation frequencies (Hz).
   * @param magnitudes       Per-point S-parameter magnitudes (dB).
   *
   * @note Silently ignored if the session is not active.
   */
  void addVnaMeasurement(double start_frequency, double stop_frequency,
                         int num_points,
                         const QVector<double>& frequencies,
                         const QVector<double>& magnitudes);

  /**
   * @brief Append a pump telemetry sample.
   *
   * @param position  Cumulative volume dispensed (µL).
   * @param flow_rate Instantaneous flow rate (µL/min).
   *
   * @note Silently ignored if the session is not active.
   */
  void addPumpSample(double position, double flow_rate);

  /**
   * @brief Append an LED state sample.
   *
   * @param power_on true if the LED output is currently active.
   * @param intensity Brightness level (0.0–100.0 %).
   *
   * @note Silently ignored if the session is not active.
   */
  void addLedSample(bool power_on, double intensity);

  /**
   * @brief Append a signal-generator parameter sample.
   *
   * @param frequency Output frequency (Hz).
   * @param amplitude Output amplitude (Vpp).
   *
   * @note Silently ignored if the session is not active.
   */
  void addSigGenSample(double frequency, double amplitude);

  /**
   * @brief Append an XYZ stage position sample.
   *
   * @param x X-axis position (mm).
   * @param y Y-axis position (mm).
   * @param z Z-axis position (mm).
   *
   * @note Silently ignored if the session is not active.
   */
  void addStageSample(double x, double y, double z);

  // -------------------------------------------------------------------------
  // Read access
  // -------------------------------------------------------------------------

  /**
   * @brief Return all camera frames accumulated in this session.
   *
   * @return Const reference to the internal frame list.
   */
  [[nodiscard]] const QVector<CameraFrame>& cameraFrames() const;

  /**
   * @brief Return all VNA measurements accumulated in this session.
   *
   * @return Const reference to the internal measurement list.
   */
  [[nodiscard]] const QVector<VnaMeasurement>& vnaMeasurements() const;

  /**
   * @brief Return all pump samples accumulated in this session.
   *
   * @return Const reference to the internal pump sample list.
   */
  [[nodiscard]] const QVector<PumpSample>& pumpSamples() const;

  /**
   * @brief Return all LED samples accumulated in this session.
   *
   * @return Const reference to the internal LED sample list.
   */
  [[nodiscard]] const QVector<LedSample>& ledSamples() const;

  /**
   * @brief Return all signal-generator samples accumulated in this session.
   *
   * @return Const reference to the internal signal-generator sample list.
   */
  [[nodiscard]] const QVector<SigGenSample>& sigGenSamples() const;

  /**
   * @brief Return all stage position samples accumulated in this session.
   *
   * @return Const reference to the internal stage sample list.
   */
  [[nodiscard]] const QVector<StageSample>& stageSamples() const;

  // -------------------------------------------------------------------------
  // Convenience counts
  // -------------------------------------------------------------------------

  /**
   * @brief Return the number of camera frames stored in this session.
   *
   * @return Frame count.
   */
  [[nodiscard]] int cameraFrameCount() const;

  /**
   * @brief Return the number of VNA measurements stored in this session.
   *
   * @return Measurement count.
   */
  [[nodiscard]] int vnaMeasurementCount() const;

  /**
   * @brief Return the number of pump samples stored in this session.
   *
   * @return Sample count.
   */
  [[nodiscard]] int pumpSampleCount() const;

  /**
   * @brief Return the number of LED samples stored in this session.
   *
   * @return Sample count.
   */
  [[nodiscard]] int ledSampleCount() const;

  /**
   * @brief Return the number of signal-generator samples stored.
   *
   * @return Sample count.
   */
  [[nodiscard]] int sigGenSampleCount() const;

  /**
   * @brief Return the number of stage position samples stored.
   *
   * @return Sample count.
   */
  [[nodiscard]] int stageSampleCount() const;

  // -------------------------------------------------------------------------
  // Reset
  // -------------------------------------------------------------------------

  /**
   * @brief Clear all accumulated data and reset metadata.
   *
   * Resets the session to its freshly-constructed idle state. Does not
   * emit sessionEnded() — use end() for a graceful close first if needed.
   */
  void clear();

 signals:
  /**
   * @brief Emitted when a session starts.
   *
   * @param name The session name passed to start().
   */
  void sessionStarted(const QString& name);

  /**
   * @brief Emitted when a session ends via end().
   */
  void sessionEnded();

  /**
   * @brief Emitted after each successful addCameraFrame() call.
   *
   * @param count Total number of frames stored so far.
   */
  void cameraFrameAdded(int count);

  /**
   * @brief Emitted after each successful addVnaMeasurement() call.
   *
   * @param count Total number of VNA measurements stored so far.
   */
  void vnaMeasurementAdded(int count);

  /**
   * @brief Emitted after each successful addPumpSample() call.
   *
   * @param count Total number of pump samples stored so far.
   */
  void pumpSampleAdded(int count);

  /**
   * @brief Emitted after each successful addLedSample() call.
   *
   * @param count Total number of LED samples stored so far.
   */
  void ledSampleAdded(int count);

  /**
   * @brief Emitted after each successful addSigGenSample() call.
   *
   * @param count Total number of signal-generator samples stored so far.
   */
  void sigGenSampleAdded(int count);

  /**
   * @brief Emitted after each successful addStageSample() call.
   *
   * @param count Total number of stage position samples stored so far.
   */
  void stageSampleAdded(int count);

 private:
  QString   name_;         ///< Session name.
  QString   description_;  ///< Optional session description.
  QDateTime start_time_;   ///< Wall-clock time when start() was called.
  QDateTime end_time_;     ///< Wall-clock time when end() was called.
  bool      is_active_{false};  ///< true while accumulating data.

  QVector<CameraFrame>    camera_frames_;     ///< Captured camera frames.
  QVector<VnaMeasurement> vna_measurements_;  ///< Completed VNA sweeps.
  QVector<PumpSample>     pump_samples_;      ///< Pump telemetry timeline.
  QVector<LedSample>      led_samples_;       ///< LED state timeline.
  QVector<SigGenSample>   sig_gen_samples_;   ///< Signal-generator timeline.
  QVector<StageSample>    stage_samples_;     ///< Stage position timeline.
};

}  // namespace mwa::analysis

/**
 * @file experiment_session.cpp
 * @brief ExperimentSession implementation.
 * @author MWA Team
 * @date 2026-04-05
 *
 * @copyright LGPL-3.0-or-later
 */

#include "analysis/experiment_session.h"

#include <QtDebug>

namespace mwa::analysis {

ExperimentSession::ExperimentSession(QObject* parent) : QObject(parent) {}

// ---------------------------------------------------------------------------
// Session lifecycle
// ---------------------------------------------------------------------------

void ExperimentSession::start(const QString& name,
                               const QString& description) {
  if (is_active_) {
    // sessionEnded is emitted here synchronously so direct-connection listeners
    // can still read accumulated data from the closing session. clear() below
    // wipes that data immediately after, so queued-connection listeners will
    // receive the signal only after the session is already empty — they must
    // not rely on reading session state in their sessionEnded handler.
    end();
  }

  clear();
  name_        = name;
  description_ = description;
  start_time_  = QDateTime::currentDateTime();
  is_active_   = true;

  emit sessionStarted(name_);
}

void ExperimentSession::end() {
  if (!is_active_) {
    return;
  }

  end_time_  = QDateTime::currentDateTime();
  is_active_ = false;

  emit sessionEnded();
}

bool ExperimentSession::isActive() const { return is_active_; }

// ---------------------------------------------------------------------------
// Metadata
// ---------------------------------------------------------------------------

QString   ExperimentSession::name()        const { return name_; }
QString   ExperimentSession::description() const { return description_; }
QDateTime ExperimentSession::startTime()   const { return start_time_; }
QDateTime ExperimentSession::endTime()     const { return end_time_; }

// ---------------------------------------------------------------------------
// Data ingestion
// ---------------------------------------------------------------------------

void ExperimentSession::addCameraFrame(const QImage& image) {
  if (!is_active_) {
    return;
  }

  camera_frames_.append(
      CameraFrame{image, QDateTime::currentDateTime()});
  emit cameraFrameAdded(camera_frames_.size());
}

void ExperimentSession::addVnaMeasurement(double start_frequency,
                                           double stop_frequency,
                                           int num_points,
                                           const QVector<double>& frequencies,
                                           const QVector<double>& magnitudes) {
  if (!is_active_) {
    return;
  }

  if (frequencies.size() != magnitudes.size() ||
      frequencies.size() != num_points) {
    qWarning() << "ExperimentSession::addVnaMeasurement: array sizes do not"
                  " match num_points — measurement discarded";
    return;
  }

  vna_measurements_.append(VnaMeasurement{
      start_frequency, stop_frequency,
      frequencies, magnitudes,
      QDateTime::currentDateTime()});
  emit vnaMeasurementAdded(vna_measurements_.size());
}

void ExperimentSession::addPumpSample(double position, double flow_rate) {
  if (!is_active_) {
    return;
  }

  pump_samples_.append(
      PumpSample{position, flow_rate, QDateTime::currentDateTime()});
  emit pumpSampleAdded(pump_samples_.size());
}

void ExperimentSession::addLedSample(bool power_on, double intensity) {
  if (!is_active_) {
    return;
  }

  led_samples_.append(
      LedSample{power_on, intensity, QDateTime::currentDateTime()});
  emit ledSampleAdded(led_samples_.size());
}

void ExperimentSession::addSigGenSample(double frequency, double amplitude) {
  if (!is_active_) {
    return;
  }

  sig_gen_samples_.append(
      SigGenSample{frequency, amplitude, QDateTime::currentDateTime()});
  emit sigGenSampleAdded(sig_gen_samples_.size());
}

void ExperimentSession::addStageSample(double x, double y, double z) {
  if (!is_active_) {
    return;
  }

  stage_samples_.append(
      StageSample{x, y, z, QDateTime::currentDateTime()});
  emit stageSampleAdded(stage_samples_.size());
}

// ---------------------------------------------------------------------------
// Read access
// ---------------------------------------------------------------------------

const QVector<CameraFrame>&    ExperimentSession::cameraFrames()    const {
  return camera_frames_;
}
const QVector<VnaMeasurement>& ExperimentSession::vnaMeasurements() const {
  return vna_measurements_;
}
const QVector<PumpSample>&     ExperimentSession::pumpSamples()     const {
  return pump_samples_;
}
const QVector<LedSample>&      ExperimentSession::ledSamples()      const {
  return led_samples_;
}
const QVector<SigGenSample>&   ExperimentSession::sigGenSamples()   const {
  return sig_gen_samples_;
}
const QVector<StageSample>&    ExperimentSession::stageSamples()    const {
  return stage_samples_;
}

// ---------------------------------------------------------------------------
// Counts
// ---------------------------------------------------------------------------

int ExperimentSession::cameraFrameCount()   const {
  return camera_frames_.size();
}
int ExperimentSession::vnaMeasurementCount() const {
  return vna_measurements_.size();
}
int ExperimentSession::pumpSampleCount()    const {
  return pump_samples_.size();
}
int ExperimentSession::ledSampleCount()     const {
  return led_samples_.size();
}
int ExperimentSession::sigGenSampleCount()  const {
  return sig_gen_samples_.size();
}
int ExperimentSession::stageSampleCount()   const {
  return stage_samples_.size();
}

// ---------------------------------------------------------------------------
// Reset
// ---------------------------------------------------------------------------

void ExperimentSession::clear() {
  name_             = {};
  description_      = {};
  start_time_       = {};
  end_time_         = {};
  is_active_        = false;
  camera_frames_.clear();
  vna_measurements_.clear();
  pump_samples_.clear();
  led_samples_.clear();
  sig_gen_samples_.clear();
  stage_samples_.clear();
}

}  // namespace mwa::analysis

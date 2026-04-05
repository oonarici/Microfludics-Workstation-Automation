/**
 * @file session_serializer.cpp
 * @brief SessionSerializer implementation.
 * @author MWA Team
 * @date 2026-04-05
 *
 * @copyright LGPL-3.0-or-later
 */

#include "analysis/session_serializer.h"

#include <QBuffer>
#include <QByteArray>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>

namespace mwa::analysis {

// ---------------------------------------------------------------------------
// Static member
// ---------------------------------------------------------------------------

QString SessionSerializer::last_error_;

// ---------------------------------------------------------------------------
// Internal constants
// ---------------------------------------------------------------------------

static constexpr int kFormatVersion = 1;

// ---------------------------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------------------------

static QString dtToStr(const QDateTime& dt) {
  return dt.isValid() ? dt.toString(Qt::ISODateWithMs) : QString{};
}

static QDateTime strToDt(const QString& s) {
  return s.isEmpty() ? QDateTime{} : QDateTime::fromString(s, Qt::ISODateWithMs);
}

static QString imageToBase64(const QImage& img) {
  QByteArray bytes;
  QBuffer buf(&bytes);
  buf.open(QIODevice::WriteOnly);
  img.save(&buf, "PNG");
  return QString::fromLatin1(bytes.toBase64());
}

static QImage imageFromBase64(const QString& b64) {
  const QByteArray bytes = QByteArray::fromBase64(b64.toLatin1());
  QImage img;
  img.loadFromData(bytes, "PNG");
  return img;
}

static QJsonArray doubleVecToJson(const QVector<double>& vec) {
  QJsonArray arr;
  for (double v : vec) {
    arr.append(v);
  }
  return arr;
}

static QVector<double> jsonToDoubleVec(const QJsonArray& arr) {
  QVector<double> vec;
  vec.reserve(arr.size());
  for (const QJsonValue& v : arr) {
    vec.append(v.toDouble());
  }
  return vec;
}

// ---------------------------------------------------------------------------
// save()
// ---------------------------------------------------------------------------

bool SessionSerializer::save(const ExperimentSession& session,
                              const QString& file_path) {
  QJsonObject root;
  root["format_version"] = kFormatVersion;
  root["name"]           = session.name_;
  root["description"]    = session.description_;
  root["start_time"]     = dtToStr(session.start_time_);
  root["end_time"]       = dtToStr(session.end_time_);
  root["is_active"]      = session.is_active_;

  // Camera frames — each frame embedded as a Base64 PNG string.
  QJsonArray frames;
  for (const CameraFrame& frame : session.camera_frames_) {
    QJsonObject obj;
    obj["timestamp"] = dtToStr(frame.timestamp);
    obj["image"]     = imageToBase64(frame.image);
    frames.append(obj);
  }
  root["camera_frames"] = frames;

  QJsonArray vna;
  for (const VnaMeasurement& m : session.vna_measurements_) {
    QJsonObject obj;
    obj["timestamp"]       = dtToStr(m.timestamp);
    obj["start_frequency"] = m.start_frequency;
    obj["stop_frequency"]  = m.stop_frequency;
    obj["frequencies"]     = doubleVecToJson(m.frequencies);
    obj["magnitudes"]      = doubleVecToJson(m.magnitudes);
    vna.append(obj);
  }
  root["vna_measurements"] = vna;

  QJsonArray pump;
  for (const PumpSample& sample : session.pump_samples_) {
    QJsonObject obj;
    obj["timestamp"] = dtToStr(sample.timestamp);
    obj["position"]  = sample.position;
    obj["flow_rate"] = sample.flow_rate;
    pump.append(obj);
  }
  root["pump_samples"] = pump;

  QJsonArray led;
  for (const LedSample& sample : session.led_samples_) {
    QJsonObject obj;
    obj["timestamp"] = dtToStr(sample.timestamp);
    obj["power_on"]  = sample.power_on;
    obj["intensity"] = sample.intensity;
    led.append(obj);
  }
  root["led_samples"] = led;

  QJsonArray siggen;
  for (const SigGenSample& sample : session.sig_gen_samples_) {
    QJsonObject obj;
    obj["timestamp"] = dtToStr(sample.timestamp);
    obj["frequency"] = sample.frequency;
    obj["amplitude"] = sample.amplitude;
    siggen.append(obj);
  }
  root["sig_gen_samples"] = siggen;

  QJsonArray stage;
  for (const StageSample& sample : session.stage_samples_) {
    QJsonObject obj;
    obj["timestamp"] = dtToStr(sample.timestamp);
    obj["x"]         = sample.x;
    obj["y"]         = sample.y;
    obj["z"]         = sample.z;
    stage.append(obj);
  }
  root["stage_samples"] = stage;

  QFile file(file_path);
  if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
    last_error_ = QStringLiteral("Cannot open file for writing: ") + file_path;
    return false;
  }

  const QJsonDocument doc(root);
  file.write(doc.toJson());

  last_error_.clear();
  return true;
}

// ---------------------------------------------------------------------------
// load()
// ---------------------------------------------------------------------------

bool SessionSerializer::load(const QString& file_path,
                              ExperimentSession& session) {
  QFile file(file_path);
  if (!file.open(QIODevice::ReadOnly)) {
    last_error_ = QStringLiteral("Cannot open file for reading: ") + file_path;
    return false;
  }

  QJsonParseError parse_err;
  const QJsonDocument doc =
      QJsonDocument::fromJson(file.readAll(), &parse_err);
  if (doc.isNull()) {
    last_error_ =
        QStringLiteral("JSON parse error: ") + parse_err.errorString();
    return false;
  }

  const QJsonObject root = doc.object();

  const int version = root.value(QStringLiteral("format_version")).toInt(0);
  if (version != kFormatVersion) {
    last_error_ =
        QStringLiteral("Unsupported format version: ") + QString::number(version);
    return false;
  }

  // Reset destination before populating — no signals emitted during restore.
  session.clear();

  session.name_        = root.value(QStringLiteral("name")).toString();
  session.description_ = root.value(QStringLiteral("description")).toString();
  session.start_time_  = strToDt(root.value(QStringLiteral("start_time")).toString());
  session.end_time_    = strToDt(root.value(QStringLiteral("end_time")).toString());
  session.is_active_   = root.value(QStringLiteral("is_active")).toBool(false);

  for (const QJsonValue& v : root.value(QStringLiteral("camera_frames")).toArray()) {
    const QJsonObject obj = v.toObject();
    CameraFrame frame;
    frame.timestamp = strToDt(obj.value(QStringLiteral("timestamp")).toString());
    frame.image     = imageFromBase64(obj.value(QStringLiteral("image")).toString());
    session.camera_frames_.append(frame);
  }

  for (const QJsonValue& v :
       root.value(QStringLiteral("vna_measurements")).toArray()) {
    const QJsonObject obj = v.toObject();
    VnaMeasurement m;
    m.timestamp       = strToDt(obj.value(QStringLiteral("timestamp")).toString());
    m.start_frequency = obj.value(QStringLiteral("start_frequency")).toDouble();
    m.stop_frequency  = obj.value(QStringLiteral("stop_frequency")).toDouble();
    m.frequencies =
        jsonToDoubleVec(obj.value(QStringLiteral("frequencies")).toArray());
    m.magnitudes =
        jsonToDoubleVec(obj.value(QStringLiteral("magnitudes")).toArray());
    session.vna_measurements_.append(m);
  }

  for (const QJsonValue& v :
       root.value(QStringLiteral("pump_samples")).toArray()) {
    const QJsonObject obj = v.toObject();
    PumpSample sample;
    sample.timestamp = strToDt(obj.value(QStringLiteral("timestamp")).toString());
    sample.position  = obj.value(QStringLiteral("position")).toDouble();
    sample.flow_rate = obj.value(QStringLiteral("flow_rate")).toDouble();
    session.pump_samples_.append(sample);
  }

  for (const QJsonValue& v :
       root.value(QStringLiteral("led_samples")).toArray()) {
    const QJsonObject obj = v.toObject();
    LedSample sample;
    sample.timestamp = strToDt(obj.value(QStringLiteral("timestamp")).toString());
    sample.power_on  = obj.value(QStringLiteral("power_on")).toBool();
    sample.intensity = obj.value(QStringLiteral("intensity")).toDouble();
    session.led_samples_.append(sample);
  }

  for (const QJsonValue& v :
       root.value(QStringLiteral("sig_gen_samples")).toArray()) {
    const QJsonObject obj = v.toObject();
    SigGenSample sample;
    sample.timestamp = strToDt(obj.value(QStringLiteral("timestamp")).toString());
    sample.frequency = obj.value(QStringLiteral("frequency")).toDouble();
    sample.amplitude = obj.value(QStringLiteral("amplitude")).toDouble();
    session.sig_gen_samples_.append(sample);
  }

  for (const QJsonValue& v :
       root.value(QStringLiteral("stage_samples")).toArray()) {
    const QJsonObject obj = v.toObject();
    StageSample sample;
    sample.timestamp = strToDt(obj.value(QStringLiteral("timestamp")).toString());
    sample.x         = obj.value(QStringLiteral("x")).toDouble();
    sample.y         = obj.value(QStringLiteral("y")).toDouble();
    sample.z         = obj.value(QStringLiteral("z")).toDouble();
    session.stage_samples_.append(sample);
  }

  last_error_.clear();
  return true;
}

// ---------------------------------------------------------------------------
// lastError()
// ---------------------------------------------------------------------------

QString SessionSerializer::lastError() { return last_error_; }

}  // namespace mwa::analysis

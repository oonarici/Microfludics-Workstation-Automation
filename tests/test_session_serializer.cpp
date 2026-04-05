/**
 * @file test_session_serializer.cpp
 * @brief Unit tests for mwa::analysis::SessionSerializer.
 */

#include <QtTest>
#include <QSignalSpy>
#include <QImage>
#include <QTemporaryFile>

#include "analysis/experiment_session.h"
#include "analysis/session_serializer.h"

using namespace mwa::analysis;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

/// Create a temporary file path (file is opened then closed; path persists).
static QString tempFilePath() {
  QTemporaryFile f;
  f.setAutoRemove(false);
  const bool opened = f.open();
  Q_ASSERT(opened);
  f.close();
  return f.fileName();
}

/// Build a small Grayscale8 image filled with @p value.
static QImage makeGrayImage(int w, int h, uchar value) {
  QImage img(w, h, QImage::Format_Grayscale8);
  img.fill(value);
  return img;
}

// ---------------------------------------------------------------------------
// Test class
// ---------------------------------------------------------------------------

class TestSessionSerializer : public QObject {
  Q_OBJECT

 private slots:

  // -------------------------------------------------------------------------
  // Round-trip: metadata
  // -------------------------------------------------------------------------

  void roundTrip_metadata() {
    ExperimentSession orig;
    orig.start("Run 42", "Acoustic focusing test");
    orig.end();

    const QString path = tempFilePath();
    QVERIFY(SessionSerializer::save(orig, path));

    ExperimentSession loaded;
    QVERIFY(SessionSerializer::load(path, loaded));

    QCOMPARE(loaded.name(),        orig.name());
    QCOMPARE(loaded.description(), orig.description());
    QVERIFY(loaded.startTime().isValid());
    QVERIFY(loaded.endTime().isValid());
    QVERIFY(!loaded.isActive());
  }

  // -------------------------------------------------------------------------
  // Round-trip: timestamps preserved to millisecond precision
  // -------------------------------------------------------------------------

  void roundTrip_timestampPrecision() {
    ExperimentSession orig;
    orig.start("Run ts");
    orig.end();

    const QString path = tempFilePath();
    QVERIFY(SessionSerializer::save(orig, path));

    ExperimentSession loaded;
    QVERIFY(SessionSerializer::load(path, loaded));

    // QDateTime::toString/fromString with ISODateWithMs should be lossless.
    QCOMPARE(loaded.startTime(), orig.startTime());
    QCOMPARE(loaded.endTime(),   orig.endTime());
  }

  // -------------------------------------------------------------------------
  // Round-trip: is_active flag preserved
  // -------------------------------------------------------------------------

  void roundTrip_isActiveFlagPreserved_whenActive() {
    ExperimentSession orig;
    orig.start("Active sess");
    // Intentionally NOT calling end() — session stays active.

    const QString path = tempFilePath();
    QVERIFY(SessionSerializer::save(orig, path));

    ExperimentSession loaded;
    QVERIFY(SessionSerializer::load(path, loaded));

    QVERIFY(loaded.isActive());
    QVERIFY(!loaded.endTime().isValid());
  }

  void roundTrip_isActiveFlagPreserved_whenEnded() {
    ExperimentSession orig;
    orig.start("Ended sess");
    orig.end();

    const QString path = tempFilePath();
    QVERIFY(SessionSerializer::save(orig, path));

    ExperimentSession loaded;
    QVERIFY(SessionSerializer::load(path, loaded));

    QVERIFY(!loaded.isActive());
  }

  // -------------------------------------------------------------------------
  // Round-trip: camera frames
  // -------------------------------------------------------------------------

  void roundTrip_cameraFrameCount() {
    ExperimentSession orig;
    orig.start("R");
    orig.addCameraFrame(makeGrayImage(4, 4, 100));
    orig.addCameraFrame(makeGrayImage(4, 4, 200));
    orig.end();

    const QString path = tempFilePath();
    QVERIFY(SessionSerializer::save(orig, path));

    ExperimentSession loaded;
    QVERIFY(SessionSerializer::load(path, loaded));

    QCOMPARE(loaded.cameraFrameCount(), 2);
  }

  void roundTrip_cameraFrameImageContent() {
    ExperimentSession orig;
    orig.start("R");
    QImage img = makeGrayImage(8, 8, 128);
    orig.addCameraFrame(img);
    orig.end();

    const QString path = tempFilePath();
    QVERIFY(SessionSerializer::save(orig, path));

    ExperimentSession loaded;
    QVERIFY(SessionSerializer::load(path, loaded));

    QCOMPARE(loaded.cameraFrameCount(), 1);
    const QImage& loaded_img = loaded.cameraFrames().at(0).image;
    QCOMPARE(loaded_img.width(),  img.width());
    QCOMPARE(loaded_img.height(), img.height());
    // Verify every pixel survived Base64 → PNG → Base64 round-trip.
    for (int y = 0; y < img.height(); ++y) {
      for (int x = 0; x < img.width(); ++x) {
        QCOMPARE(loaded_img.pixel(x, y), img.pixel(x, y));
      }
    }
  }

  void roundTrip_cameraFrameTimestamp() {
    ExperimentSession orig;
    orig.start("R");
    orig.addCameraFrame(makeGrayImage(2, 2, 0));
    orig.end();

    const QString path = tempFilePath();
    QVERIFY(SessionSerializer::save(orig, path));

    ExperimentSession loaded;
    QVERIFY(SessionSerializer::load(path, loaded));

    QVERIFY(loaded.cameraFrames().at(0).timestamp.isValid());
    QCOMPARE(loaded.cameraFrames().at(0).timestamp,
             orig.cameraFrames().at(0).timestamp);
  }

  // -------------------------------------------------------------------------
  // Round-trip: VNA measurements
  // -------------------------------------------------------------------------

  void roundTrip_vnaMeasurements() {
    ExperimentSession orig;
    orig.start("R");
    QVector<double> freqs = {1e9, 1.5e9, 2e9};
    QVector<double> mags  = {-40.0, -35.0, -42.5};
    orig.addVnaMeasurement(1e9, 2e9, 3, freqs, mags);
    orig.end();

    const QString path = tempFilePath();
    QVERIFY(SessionSerializer::save(orig, path));

    ExperimentSession loaded;
    QVERIFY(SessionSerializer::load(path, loaded));

    QCOMPARE(loaded.vnaMeasurementCount(), 1);
    const VnaMeasurement& m = loaded.vnaMeasurements().at(0);
    QCOMPARE(m.start_frequency, 1e9);
    QCOMPARE(m.stop_frequency,  2e9);
    QCOMPARE(m.frequencies, freqs);
    QCOMPARE(m.magnitudes,  mags);
    QVERIFY(m.timestamp.isValid());
    QCOMPARE(m.timestamp, orig.vnaMeasurements().at(0).timestamp);
  }

  // -------------------------------------------------------------------------
  // Round-trip: pump samples
  // -------------------------------------------------------------------------

  void roundTrip_pumpSamples() {
    ExperimentSession orig;
    orig.start("R");
    orig.addPumpSample(12.5, 3.0);
    orig.addPumpSample(25.0, 6.0);
    orig.end();

    const QString path = tempFilePath();
    QVERIFY(SessionSerializer::save(orig, path));

    ExperimentSession loaded;
    QVERIFY(SessionSerializer::load(path, loaded));

    QCOMPARE(loaded.pumpSampleCount(), 2);
    QCOMPARE(loaded.pumpSamples().at(0).position,  12.5);
    QCOMPARE(loaded.pumpSamples().at(0).flow_rate,  3.0);
    QCOMPARE(loaded.pumpSamples().at(1).position,  25.0);
    QCOMPARE(loaded.pumpSamples().at(1).flow_rate,  6.0);
    QVERIFY(loaded.pumpSamples().at(0).timestamp.isValid());
  }

  // -------------------------------------------------------------------------
  // Round-trip: LED samples
  // -------------------------------------------------------------------------

  void roundTrip_ledSamples() {
    ExperimentSession orig;
    orig.start("R");
    orig.addLedSample(true,  75.0);
    orig.addLedSample(false, 0.0);
    orig.end();

    const QString path = tempFilePath();
    QVERIFY(SessionSerializer::save(orig, path));

    ExperimentSession loaded;
    QVERIFY(SessionSerializer::load(path, loaded));

    QCOMPARE(loaded.ledSampleCount(), 2);
    QVERIFY(loaded.ledSamples().at(0).power_on);
    QCOMPARE(loaded.ledSamples().at(0).intensity, 75.0);
    QVERIFY(!loaded.ledSamples().at(1).power_on);
    QCOMPARE(loaded.ledSamples().at(1).intensity, 0.0);
  }

  // -------------------------------------------------------------------------
  // Round-trip: signal-generator samples
  // -------------------------------------------------------------------------

  void roundTrip_sigGenSamples() {
    ExperimentSession orig;
    orig.start("R");
    orig.addSigGenSample(1e6, 2.5);
    orig.end();

    const QString path = tempFilePath();
    QVERIFY(SessionSerializer::save(orig, path));

    ExperimentSession loaded;
    QVERIFY(SessionSerializer::load(path, loaded));

    QCOMPARE(loaded.sigGenSampleCount(), 1);
    QCOMPARE(loaded.sigGenSamples().at(0).frequency, 1e6);
    QCOMPARE(loaded.sigGenSamples().at(0).amplitude, 2.5);
    QVERIFY(loaded.sigGenSamples().at(0).timestamp.isValid());
  }

  // -------------------------------------------------------------------------
  // Round-trip: stage samples
  // -------------------------------------------------------------------------

  void roundTrip_stageSamples() {
    ExperimentSession orig;
    orig.start("R");
    orig.addStageSample(1.5, 2.5, 0.1);
    orig.end();

    const QString path = tempFilePath();
    QVERIFY(SessionSerializer::save(orig, path));

    ExperimentSession loaded;
    QVERIFY(SessionSerializer::load(path, loaded));

    QCOMPARE(loaded.stageSampleCount(), 1);
    QCOMPARE(loaded.stageSamples().at(0).x, 1.5);
    QCOMPARE(loaded.stageSamples().at(0).y, 2.5);
    QCOMPARE(loaded.stageSamples().at(0).z, 0.1);
  }

  // -------------------------------------------------------------------------
  // Round-trip: all device types simultaneously
  // -------------------------------------------------------------------------

  void roundTrip_allDeviceTypes() {
    ExperimentSession orig;
    orig.start("Full run", "All devices active");

    for (int i = 0; i < 3; ++i)
      orig.addCameraFrame(makeGrayImage(2, 2, static_cast<uchar>(i * 40)));
    orig.addVnaMeasurement(1e9, 2e9, 2, {1e9, 2e9}, {-40.0, -38.0});
    orig.addPumpSample(5.0, 1.0);
    orig.addLedSample(true, 50.0);
    orig.addSigGenSample(2e6, 3.0);
    orig.addStageSample(0.5, 1.0, 0.2);

    orig.end();

    const QString path = tempFilePath();
    QVERIFY(SessionSerializer::save(orig, path));

    ExperimentSession loaded;
    QVERIFY(SessionSerializer::load(path, loaded));

    QCOMPARE(loaded.cameraFrameCount(),    3);
    QCOMPARE(loaded.vnaMeasurementCount(), 1);
    QCOMPARE(loaded.pumpSampleCount(),     1);
    QCOMPARE(loaded.ledSampleCount(),      1);
    QCOMPARE(loaded.sigGenSampleCount(),   1);
    QCOMPARE(loaded.stageSampleCount(),    1);
    QCOMPARE(loaded.name(),        orig.name());
    QCOMPARE(loaded.description(), orig.description());
  }

  // -------------------------------------------------------------------------
  // Round-trip: empty session
  // -------------------------------------------------------------------------

  void roundTrip_emptySession() {
    ExperimentSession orig;
    orig.start("Empty");
    orig.end();

    const QString path = tempFilePath();
    QVERIFY(SessionSerializer::save(orig, path));

    ExperimentSession loaded;
    QVERIFY(SessionSerializer::load(path, loaded));

    QCOMPARE(loaded.name(), QString("Empty"));
    QCOMPARE(loaded.cameraFrameCount(),    0);
    QCOMPARE(loaded.vnaMeasurementCount(), 0);
    QCOMPARE(loaded.pumpSampleCount(),     0);
    QCOMPARE(loaded.ledSampleCount(),      0);
    QCOMPARE(loaded.sigGenSampleCount(),   0);
    QCOMPARE(loaded.stageSampleCount(),    0);
  }

  // -------------------------------------------------------------------------
  // load() clears pre-existing data in the destination session
  // -------------------------------------------------------------------------

  void load_clearsExistingDataBeforePopulating() {
    // Populate the destination session with stale data.
    ExperimentSession dest;
    dest.start("Stale");
    dest.addCameraFrame(makeGrayImage(1, 1, 0));
    dest.addPumpSample(99.0, 99.0);
    dest.end();
    QCOMPARE(dest.cameraFrameCount(), 1);

    // Save a different (smaller) session.
    ExperimentSession fresh;
    fresh.start("Fresh");
    fresh.end();
    const QString path = tempFilePath();
    QVERIFY(SessionSerializer::save(fresh, path));

    // Loading must wipe the stale data.
    QVERIFY(SessionSerializer::load(path, dest));
    QCOMPARE(dest.name(),             QString("Fresh"));
    QCOMPARE(dest.cameraFrameCount(), 0);
    QCOMPARE(dest.pumpSampleCount(),  0);
  }

  // -------------------------------------------------------------------------
  // load() emits no signals on the destination session
  // -------------------------------------------------------------------------

  void load_emitsNoSignals() {
    ExperimentSession orig;
    orig.start("Sig test");
    orig.end();
    const QString path = tempFilePath();
    QVERIFY(SessionSerializer::save(orig, path));

    ExperimentSession dest;
    QSignalSpy started(&dest, &ExperimentSession::sessionStarted);
    QSignalSpy ended  (&dest, &ExperimentSession::sessionEnded);

    QVERIFY(SessionSerializer::load(path, dest));

    QCOMPARE(started.count(), 0);
    QCOMPARE(ended.count(),   0);
  }

  // -------------------------------------------------------------------------
  // Error handling: non-existent file
  // -------------------------------------------------------------------------

  void load_failsOnNonExistentFile() {
    ExperimentSession dest;
    const bool ok = SessionSerializer::load(
        "/tmp/mwa_test_nonexistent_file_xyz.json", dest);

    QVERIFY(!ok);
    QVERIFY(!SessionSerializer::lastError().isEmpty());
  }

  // -------------------------------------------------------------------------
  // Error handling: corrupt JSON
  // -------------------------------------------------------------------------

  void load_failsOnCorruptJson() {
    QTemporaryFile tmp;
    const bool c1 = tmp.open();
    Q_ASSERT(c1);
    tmp.write("{ this is not valid json !!!");
    tmp.close();

    ExperimentSession dest;
    const bool ok = SessionSerializer::load(tmp.fileName(), dest);

    QVERIFY(!ok);
    QVERIFY(!SessionSerializer::lastError().isEmpty());
  }

  // -------------------------------------------------------------------------
  // Error handling: wrong format_version
  // -------------------------------------------------------------------------

  void load_failsOnWrongFormatVersion() {
    QTemporaryFile tmp;
    const bool c2 = tmp.open();
    Q_ASSERT(c2);
    tmp.write(R"({"format_version": 999, "name": "x"})");
    tmp.close();

    ExperimentSession dest;
    const bool ok = SessionSerializer::load(tmp.fileName(), dest);

    QVERIFY(!ok);
    QVERIFY(!SessionSerializer::lastError().isEmpty());
  }

  // -------------------------------------------------------------------------
  // Error handling: save to unwritable path
  // -------------------------------------------------------------------------

  void save_failsOnUnwritablePath() {
    ExperimentSession sess;
    sess.start("R");
    sess.end();

    const bool ok = SessionSerializer::save(
        sess, "/nonexistent_directory/mwa_test.json");

    QVERIFY(!ok);
    QVERIFY(!SessionSerializer::lastError().isEmpty());
  }

  // -------------------------------------------------------------------------
  // lastError() is cleared on success
  // -------------------------------------------------------------------------

  void lastError_clearedAfterSuccess() {
    // Trigger a failure first to populate lastError().
    ExperimentSession dest;
    SessionSerializer::load("/tmp/mwa_no_such_file.json", dest);
    QVERIFY(!SessionSerializer::lastError().isEmpty());

    // Successful save + load should clear it.
    ExperimentSession orig;
    orig.start("R");
    orig.end();
    const QString path = tempFilePath();
    QVERIFY(SessionSerializer::save(orig, path));
    QVERIFY(SessionSerializer::lastError().isEmpty());

    QVERIFY(SessionSerializer::load(path, dest));
    QVERIFY(SessionSerializer::lastError().isEmpty());
  }
};

QTEST_MAIN(TestSessionSerializer)
#include "test_session_serializer.moc"

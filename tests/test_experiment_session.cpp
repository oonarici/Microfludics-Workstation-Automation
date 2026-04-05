/**
 * @file test_experiment_session.cpp
 * @brief Unit tests for mwa::analysis::ExperimentSession.
 */

#include <QtTest>
#include <QSignalSpy>
#include <QImage>

#include "analysis/experiment_session.h"

using namespace mwa::analysis;

class TestExperimentSession : public QObject {
  Q_OBJECT

 private slots:
  // -------------------------------------------------------------------------
  // Lifecycle
  // -------------------------------------------------------------------------

  void initialStateIsIdle() {
    ExperimentSession s;
    QVERIFY(!s.isActive());
    QVERIFY(s.name().isEmpty());
    QVERIFY(s.description().isEmpty());
    QVERIFY(!s.startTime().isValid());
    QVERIFY(!s.endTime().isValid());
  }

  void startActivatesSession() {
    ExperimentSession s;
    s.start("Run 1");
    QVERIFY(s.isActive());
    QCOMPARE(s.name(), QString("Run 1"));
    QVERIFY(s.startTime().isValid());
    QVERIFY(!s.endTime().isValid());
  }

  void startWithDescription() {
    ExperimentSession s;
    s.start("Run 2", "Acoustic focusing test");
    QCOMPARE(s.description(), QString("Acoustic focusing test"));
  }

  void endDeactivatesSession() {
    ExperimentSession s;
    s.start("Run 1");
    s.end();
    QVERIFY(!s.isActive());
    QVERIFY(s.endTime().isValid());
    QVERIFY(s.endTime() >= s.startTime());
  }

  void endOnIdleSessionIsNoOp() {
    ExperimentSession s;
    // Must not crash or change state
    s.end();
    QVERIFY(!s.isActive());
    QVERIFY(!s.endTime().isValid());
  }

  void startWhileActiveEndsFirstSession() {
    ExperimentSession s;

    QSignalSpy ended(&s, &ExperimentSession::sessionEnded);
    QSignalSpy started(&s, &ExperimentSession::sessionStarted);

    s.start("Run 1");
    s.start("Run 2");  // should end Run 1 first

    QCOMPARE(ended.count(), 1);
    QCOMPARE(started.count(), 2);
    QCOMPARE(s.name(), QString("Run 2"));
  }

  void startWhileActiveClearsPreviousData() {
    ExperimentSession s;
    s.start("Run 1");
    s.addCameraFrame(QImage(1, 1, QImage::Format_Grayscale8));
    s.addPumpSample(5.0, 1.0);
    QCOMPARE(s.cameraFrameCount(), 1);

    s.start("Run 2");

    QCOMPARE(s.cameraFrameCount(), 0);
    QCOMPARE(s.pumpSampleCount(), 0);
    QVERIFY(s.isActive());
    QCOMPARE(s.name(), QString("Run 2"));
  }

  // -------------------------------------------------------------------------
  // Signals from lifecycle
  // -------------------------------------------------------------------------

  void sessionStartedSignalCarriesName() {
    ExperimentSession s;
    QSignalSpy spy(&s, &ExperimentSession::sessionStarted);
    s.start("MySess");
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.at(0).at(0).toString(), QString("MySess"));
  }

  void sessionEndedSignalEmittedOnce() {
    ExperimentSession s;
    QSignalSpy spy(&s, &ExperimentSession::sessionEnded);
    s.start("R");
    s.end();
    QCOMPARE(spy.count(), 1);
  }

  // -------------------------------------------------------------------------
  // Data ingestion: addCameraFrame
  // -------------------------------------------------------------------------

  void cameraFrameNotAcceptedWhenIdle() {
    ExperimentSession s;
    s.addCameraFrame(QImage(10, 10, QImage::Format_Grayscale8));
    QCOMPARE(s.cameraFrameCount(), 0);
  }

  void cameraFrameStoredWhenActive() {
    ExperimentSession s;
    s.start("R");
    QImage img(640, 480, QImage::Format_Grayscale8);
    img.fill(128);
    s.addCameraFrame(img);
    QCOMPARE(s.cameraFrameCount(), 1);
    QCOMPARE(s.cameraFrames().at(0).image.width(), 640);
    QCOMPARE(s.cameraFrames().at(0).image.height(), 480);
    QVERIFY(s.cameraFrames().at(0).timestamp.isValid());
  }

  void cameraFrameCountSignalEmittedWithCorrectCount() {
    ExperimentSession s;
    s.start("R");
    QSignalSpy spy(&s, &ExperimentSession::cameraFrameAdded);

    s.addCameraFrame(QImage(1, 1, QImage::Format_Grayscale8));
    s.addCameraFrame(QImage(1, 1, QImage::Format_Grayscale8));
    s.addCameraFrame(QImage(1, 1, QImage::Format_Grayscale8));

    QCOMPARE(spy.count(), 3);
    QCOMPARE(spy.at(0).at(0).toInt(), 1);
    QCOMPARE(spy.at(1).at(0).toInt(), 2);
    QCOMPARE(spy.at(2).at(0).toInt(), 3);
  }

  // -------------------------------------------------------------------------
  // Data ingestion: addVnaMeasurement
  // -------------------------------------------------------------------------

  void vnaMeasurementNotAcceptedWhenIdle() {
    ExperimentSession s;
    s.addVnaMeasurement({1e9, 2e9}, {-40.0, -38.0}, 1e9, 2e9, 2);
    QCOMPARE(s.vnaMeasurementCount(), 0);
  }

  void vnaMeasurementStoredWhenActive() {
    ExperimentSession s;
    s.start("R");
    QVector<double> freqs = {1e9, 2e9, 3e9};
    QVector<double> mags  = {-40.0, -35.0, -42.0};
    s.addVnaMeasurement(freqs, mags, 1e9, 3e9, 3);

    QCOMPARE(s.vnaMeasurementCount(), 1);
    const auto& m = s.vnaMeasurements().at(0);
    QCOMPARE(m.start_frequency, 1e9);
    QCOMPARE(m.stop_frequency, 3e9);
    QCOMPARE(m.frequencies, freqs);
    QCOMPARE(m.magnitudes, mags);
    QVERIFY(m.timestamp.isValid());
  }

  void vnaMeasurementDiscardedOnLengthMismatch() {
    ExperimentSession s;
    s.start("R");
    QSignalSpy spy(&s, &ExperimentSession::vnaMeasurementAdded);

    // frequencies has 3 elements, magnitudes has 2 — must be rejected
    s.addVnaMeasurement({1e9, 2e9, 3e9}, {-40.0, -35.0}, 1e9, 3e9, 3);

    QCOMPARE(s.vnaMeasurementCount(), 0);
    QCOMPARE(spy.count(), 0);
  }

  void vnaMeasurementDiscardedWhenNumPointsMismatch() {
    ExperimentSession s;
    s.start("R");
    QSignalSpy spy(&s, &ExperimentSession::vnaMeasurementAdded);

    // arrays match each other but num_points disagrees — must be rejected
    s.addVnaMeasurement({1e9, 2e9}, {-40.0, -35.0}, 1e9, 2e9, 100);

    QCOMPARE(s.vnaMeasurementCount(), 0);
    QCOMPARE(spy.count(), 0);
  }

  void vnaMeasurementSignalEmittedWithCorrectCount() {
    ExperimentSession s;
    s.start("R");
    QSignalSpy spy(&s, &ExperimentSession::vnaMeasurementAdded);

    s.addVnaMeasurement({1e9}, {-40.0}, 1e9, 1e9, 1);
    s.addVnaMeasurement({2e9}, {-38.0}, 2e9, 2e9, 1);

    QCOMPARE(spy.count(), 2);
    QCOMPARE(spy.at(0).at(0).toInt(), 1);
    QCOMPARE(spy.at(1).at(0).toInt(), 2);
  }

  // -------------------------------------------------------------------------
  // Data ingestion: addPumpSample
  // -------------------------------------------------------------------------

  void pumpSampleNotAcceptedWhenIdle() {
    ExperimentSession s;
    s.addPumpSample(10.0, 5.0);
    QCOMPARE(s.pumpSampleCount(), 0);
  }

  void pumpSampleStoredWhenActive() {
    ExperimentSession s;
    s.start("R");
    s.addPumpSample(12.5, 3.0);
    QCOMPARE(s.pumpSampleCount(), 1);
    QCOMPARE(s.pumpSamples().at(0).position, 12.5);
    QCOMPARE(s.pumpSamples().at(0).flow_rate, 3.0);
    QVERIFY(s.pumpSamples().at(0).timestamp.isValid());
  }

  // -------------------------------------------------------------------------
  // Data ingestion: addLedSample
  // -------------------------------------------------------------------------

  void ledSampleNotAcceptedWhenIdle() {
    ExperimentSession s;
    s.addLedSample(true, 75.0);
    QCOMPARE(s.ledSampleCount(), 0);
  }

  void ledSampleStoredWhenActive() {
    ExperimentSession s;
    s.start("R");
    s.addLedSample(true, 75.0);
    QCOMPARE(s.ledSampleCount(), 1);
    QVERIFY(s.ledSamples().at(0).power_on);
    QCOMPARE(s.ledSamples().at(0).intensity, 75.0);
    QVERIFY(s.ledSamples().at(0).timestamp.isValid());
  }

  // -------------------------------------------------------------------------
  // Data ingestion: addSigGenSample
  // -------------------------------------------------------------------------

  void sigGenSampleNotAcceptedWhenIdle() {
    ExperimentSession s;
    s.addSigGenSample(1e6, 2.5);
    QCOMPARE(s.sigGenSampleCount(), 0);
  }

  void sigGenSampleStoredWhenActive() {
    ExperimentSession s;
    s.start("R");
    s.addSigGenSample(1e6, 2.5);
    QCOMPARE(s.sigGenSampleCount(), 1);
    QCOMPARE(s.sigGenSamples().at(0).frequency, 1e6);
    QCOMPARE(s.sigGenSamples().at(0).amplitude, 2.5);
    QVERIFY(s.sigGenSamples().at(0).timestamp.isValid());
  }

  // -------------------------------------------------------------------------
  // Data ingestion: addStageSample
  // -------------------------------------------------------------------------

  void stageSampleNotAcceptedWhenIdle() {
    ExperimentSession s;
    s.addStageSample(1.0, 2.0, 3.0);
    QCOMPARE(s.stageSampleCount(), 0);
  }

  void stageSampleStoredWhenActive() {
    ExperimentSession s;
    s.start("R");
    s.addStageSample(1.5, 2.5, 0.1);
    QCOMPARE(s.stageSampleCount(), 1);
    QCOMPARE(s.stageSamples().at(0).x, 1.5);
    QCOMPARE(s.stageSamples().at(0).y, 2.5);
    QCOMPARE(s.stageSamples().at(0).z, 0.1);
    QVERIFY(s.stageSamples().at(0).timestamp.isValid());
  }

  // -------------------------------------------------------------------------
  // No data accepted after end()
  // -------------------------------------------------------------------------

  void noDataAcceptedAfterEnd() {
    ExperimentSession s;
    s.start("R");
    s.end();

    s.addCameraFrame(QImage(1, 1, QImage::Format_Grayscale8));
    s.addVnaMeasurement({1e9}, {-40.0}, 1e9, 1e9, 1);
    s.addPumpSample(1.0, 1.0);
    s.addLedSample(true, 50.0);
    s.addSigGenSample(1e6, 1.0);
    s.addStageSample(0.0, 0.0, 0.0);

    QCOMPARE(s.cameraFrameCount(), 0);
    QCOMPARE(s.vnaMeasurementCount(), 0);
    QCOMPARE(s.pumpSampleCount(), 0);
    QCOMPARE(s.ledSampleCount(), 0);
    QCOMPARE(s.sigGenSampleCount(), 0);
    QCOMPARE(s.stageSampleCount(), 0);
  }

  // -------------------------------------------------------------------------
  // clear()
  // -------------------------------------------------------------------------

  void clearResetsEverything() {
    ExperimentSession s;
    s.start("R");
    s.addCameraFrame(QImage(1, 1, QImage::Format_Grayscale8));
    s.addPumpSample(5.0, 2.0);
    s.end();

    s.clear();

    QVERIFY(!s.isActive());
    QVERIFY(s.name().isEmpty());
    QVERIFY(s.description().isEmpty());
    QVERIFY(!s.startTime().isValid());
    QVERIFY(!s.endTime().isValid());
    QCOMPARE(s.cameraFrameCount(), 0);
    QCOMPARE(s.pumpSampleCount(), 0);
  }

  // -------------------------------------------------------------------------
  // Data persists across start→end without clear
  // -------------------------------------------------------------------------

  void endDoesNotClearData() {
    ExperimentSession s;
    s.start("R");
    s.addCameraFrame(QImage(1, 1, QImage::Format_Grayscale8));
    s.end();

    // After end() data must still be readable
    QCOMPARE(s.cameraFrameCount(), 1);
  }

  // -------------------------------------------------------------------------
  // Multiple devices simultaneously
  // -------------------------------------------------------------------------

  void allDeviceTypesAccumulateIndependently() {
    ExperimentSession s;
    s.start("Multi");

    for (int i = 0; i < 5; ++i)
      s.addCameraFrame(QImage(1, 1, QImage::Format_Grayscale8));
    for (int i = 0; i < 3; ++i)
      s.addVnaMeasurement({1e9}, {-40.0}, 1e9, 1e9, 1);
    for (int i = 0; i < 7; ++i)
      s.addPumpSample(static_cast<double>(i), 1.0);
    s.addLedSample(true, 50.0);
    s.addSigGenSample(1e6, 1.0);
    s.addStageSample(0.0, 0.0, 0.0);

    QCOMPARE(s.cameraFrameCount(), 5);
    QCOMPARE(s.vnaMeasurementCount(), 3);
    QCOMPARE(s.pumpSampleCount(), 7);
    QCOMPARE(s.ledSampleCount(), 1);
    QCOMPARE(s.sigGenSampleCount(), 1);
    QCOMPARE(s.stageSampleCount(), 1);
  }
};

QTEST_MAIN(TestExperimentSession)
#include "test_experiment_session.moc"

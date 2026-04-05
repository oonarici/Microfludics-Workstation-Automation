/**
 * @file test_vna_csv_exporter.cpp
 * @brief Unit tests for mwa::analysis::VnaCsvExporter.
 */

#include <QtTest>
#include <QFile>
#include <QTemporaryFile>
#include <QTextStream>

#include "analysis/experiment_session.h"
#include "analysis/vna_csv_exporter.h"

using namespace mwa::analysis;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

/// Create a temporary file path (file opened then closed; path persists).
static QString tempFilePath() {
  QTemporaryFile f;
  f.setAutoRemove(false);
  const bool opened = f.open();
  Q_ASSERT(opened);
  f.close();
  return f.fileName();
}

/// Read all lines from @p path, stripping trailing newlines.
static QStringList readLines(const QString& path) {
  QFile file(path);
  if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    return {};
  QTextStream in(&file);
  QStringList lines;
  while (!in.atEnd()) {
    const QString line = in.readLine();
    if (!line.isEmpty())
      lines.append(line);
  }
  return lines;
}

// ---------------------------------------------------------------------------
// Test class
// ---------------------------------------------------------------------------

class TestVnaCsvExporter : public QObject {
  Q_OBJECT

 private slots:

  // -------------------------------------------------------------------------
  // Empty session: only header row written
  // -------------------------------------------------------------------------

  void exportToFile_emptySession_writesHeaderOnly() {
    ExperimentSession session;
    session.start("Empty");
    session.end();

    const QString path = tempFilePath();
    QVERIFY(VnaCsvExporter::exportToFile(session, path));
    QVERIFY(VnaCsvExporter::lastError().isEmpty());

    const QStringList lines = readLines(path);
    // Exactly one line: the header.
    QCOMPARE(lines.size(), 1);
    QVERIFY(lines.at(0).startsWith(
        QStringLiteral("sweep_index,timestamp")));
  }

  // -------------------------------------------------------------------------
  // Header format: correct column names
  // -------------------------------------------------------------------------

  void exportToFile_headerContainsAllColumns() {
    ExperimentSession session;
    session.start("H");
    session.end();

    const QString path = tempFilePath();
    QVERIFY(VnaCsvExporter::exportToFile(session, path));

    const QStringList lines = readLines(path);
    QVERIFY(!lines.isEmpty());
    const QStringList cols = lines.at(0).split(',');
    QCOMPARE(cols.size(), 6);
    QCOMPARE(cols.at(0), QStringLiteral("sweep_index"));
    QCOMPARE(cols.at(1), QStringLiteral("timestamp"));
    QCOMPARE(cols.at(2), QStringLiteral("start_frequency_hz"));
    QCOMPARE(cols.at(3), QStringLiteral("stop_frequency_hz"));
    QCOMPARE(cols.at(4), QStringLiteral("frequency_hz"));
    QCOMPARE(cols.at(5), QStringLiteral("magnitude_db"));
  }

  // -------------------------------------------------------------------------
  // Single sweep: row count = num_points + 1 (header)
  // -------------------------------------------------------------------------

  void exportToFile_singleSweep_rowCountMatchesNumPoints() {
    ExperimentSession session;
    session.start("S1");
    const QVector<double> freqs = {1e6, 2e6, 3e6, 4e6, 5e6};
    const QVector<double> mags  = {-10.0, -12.0, -11.5, -13.0, -9.5};
    session.addVnaMeasurement(1e6, 5e6, 5, freqs, mags);
    session.end();

    const QString path = tempFilePath();
    QVERIFY(VnaCsvExporter::exportToFile(session, path));

    const QStringList lines = readLines(path);
    // 1 header + 5 data rows.
    QCOMPARE(lines.size(), 6);
  }

  // -------------------------------------------------------------------------
  // Single sweep: sweep_index is always 0
  // -------------------------------------------------------------------------

  void exportToFile_singleSweep_sweepIndexIsZero() {
    ExperimentSession session;
    session.start("S1");
    const QVector<double> freqs = {1e6, 2e6};
    const QVector<double> mags  = {-10.0, -11.0};
    session.addVnaMeasurement(1e6, 2e6, 2, freqs, mags);
    session.end();

    const QString path = tempFilePath();
    QVERIFY(VnaCsvExporter::exportToFile(session, path));

    const QStringList lines = readLines(path);
    for (int i = 1; i < lines.size(); ++i) {
      const QStringList cols = lines.at(i).split(',');
      QCOMPARE(cols.at(0), QStringLiteral("0"));
    }
  }

  // -------------------------------------------------------------------------
  // Multiple sweeps: total rows = sum of all points + 1 (header)
  // -------------------------------------------------------------------------

  void exportToFile_multiSweep_totalRowCount() {
    ExperimentSession session;
    session.start("M");
    // Sweep 0: 3 points
    session.addVnaMeasurement(1e6, 3e6, 3,
                              {1e6, 2e6, 3e6},
                              {-10.0, -11.0, -12.0});
    // Sweep 1: 2 points
    session.addVnaMeasurement(4e6, 5e6, 2,
                              {4e6, 5e6},
                              {-13.0, -14.0});
    session.end();

    const QString path = tempFilePath();
    QVERIFY(VnaCsvExporter::exportToFile(session, path));

    const QStringList lines = readLines(path);
    // 1 header + 3 + 2 = 6 lines.
    QCOMPARE(lines.size(), 6);
  }

  // -------------------------------------------------------------------------
  // Multiple sweeps: sweep_index increments per sweep
  // -------------------------------------------------------------------------

  void exportToFile_multiSweep_sweepIndexIncrements() {
    ExperimentSession session;
    session.start("M");
    session.addVnaMeasurement(1e6, 2e6, 2, {1e6, 2e6}, {-10.0, -11.0});
    session.addVnaMeasurement(3e6, 4e6, 2, {3e6, 4e6}, {-12.0, -13.0});
    session.end();

    const QString path = tempFilePath();
    QVERIFY(VnaCsvExporter::exportToFile(session, path));

    const QStringList lines = readLines(path);
    // lines[1], lines[2] → sweep 0; lines[3], lines[4] → sweep 1.
    QCOMPARE(lines.at(1).split(',').at(0), QStringLiteral("0"));
    QCOMPARE(lines.at(2).split(',').at(0), QStringLiteral("0"));
    QCOMPARE(lines.at(3).split(',').at(0), QStringLiteral("1"));
    QCOMPARE(lines.at(4).split(',').at(0), QStringLiteral("1"));
  }

  // -------------------------------------------------------------------------
  // Values round-trip: start/stop frequency and magnitude preserved
  // -------------------------------------------------------------------------

  void exportToFile_singlePoint_valuesMatchInput() {
    ExperimentSession session;
    session.start("V");
    session.addVnaMeasurement(1000000.0, 10000000.0, 1,
                              {5000000.0},
                              {-25.5});
    session.end();

    const QString path = tempFilePath();
    QVERIFY(VnaCsvExporter::exportToFile(session, path));

    const QStringList lines = readLines(path);
    QCOMPARE(lines.size(), 2);

    const QStringList cols = lines.at(1).split(',');
    QCOMPARE(cols.size(), 6);
    QCOMPARE(cols.at(0), QStringLiteral("0"));
    // start_frequency_hz — written as integer (f,0)
    QCOMPARE(cols.at(2), QStringLiteral("1000000"));
    QCOMPARE(cols.at(3), QStringLiteral("10000000"));
    // frequency_hz and magnitude_db — written with 6 decimal places
    QCOMPARE(cols.at(4), QStringLiteral("5000000.000000"));
    QCOMPARE(cols.at(5), QStringLiteral("-25.500000"));
  }

  // -------------------------------------------------------------------------
  // Timestamp: present and non-empty in each data row
  // -------------------------------------------------------------------------

  void exportToFile_dataRows_timestampNonEmpty() {
    ExperimentSession session;
    session.start("T");
    session.addVnaMeasurement(1e6, 2e6, 2, {1e6, 2e6}, {-10.0, -11.0});
    session.end();

    const QString path = tempFilePath();
    QVERIFY(VnaCsvExporter::exportToFile(session, path));

    const QStringList lines = readLines(path);
    for (int i = 1; i < lines.size(); ++i) {
      const QStringList cols = lines.at(i).split(',');
      QVERIFY2(!cols.at(1).isEmpty(), "timestamp column must not be empty");
    }
  }

  // -------------------------------------------------------------------------
  // Multiple sweeps: each sweep's rows share the same timestamp
  // -------------------------------------------------------------------------

  void exportToFile_sweepRows_shareTimestamp() {
    ExperimentSession session;
    session.start("TS");
    session.addVnaMeasurement(1e6, 3e6, 3,
                              {1e6, 2e6, 3e6},
                              {-10.0, -11.0, -12.0});
    session.end();

    const QString path = tempFilePath();
    QVERIFY(VnaCsvExporter::exportToFile(session, path));

    const QStringList lines = readLines(path);
    // All three data rows must have the same timestamp (column 1).
    const QString ts0 = lines.at(1).split(',').at(1);
    QCOMPARE(lines.at(2).split(',').at(1), ts0);
    QCOMPARE(lines.at(3).split(',').at(1), ts0);
  }

  // -------------------------------------------------------------------------
  // Error handling: unwritable path
  // -------------------------------------------------------------------------

  void exportToFile_failsOnUnwritablePath() {
    ExperimentSession session;
    session.start("R");
    session.end();

    const bool ok = VnaCsvExporter::exportToFile(
        session, QStringLiteral("/nonexistent_dir/mwa_test.csv"));

    QVERIFY(!ok);
    QVERIFY(!VnaCsvExporter::lastError().isEmpty());
  }

  // -------------------------------------------------------------------------
  // lastError() is cleared on success
  // -------------------------------------------------------------------------

  void lastError_clearedAfterSuccess() {
    // Trigger a failure first.
    ExperimentSession session;
    session.start("R");
    session.end();
    VnaCsvExporter::exportToFile(
        session, QStringLiteral("/nonexistent_dir/mwa_test.csv"));
    QVERIFY(!VnaCsvExporter::lastError().isEmpty());

    // Successful export must clear it.
    const QString path = tempFilePath();
    QVERIFY(VnaCsvExporter::exportToFile(session, path));
    QVERIFY(VnaCsvExporter::lastError().isEmpty());
  }

  // -------------------------------------------------------------------------
  // Overwrites existing file
  // -------------------------------------------------------------------------

  void exportToFile_overwritesExistingFile() {
    ExperimentSession s1;
    s1.start("S1");
    s1.addVnaMeasurement(1e6, 2e6, 2, {1e6, 2e6}, {-10.0, -11.0});
    s1.end();

    const QString path = tempFilePath();
    QVERIFY(VnaCsvExporter::exportToFile(s1, path));
    const int first_line_count = readLines(path).size();

    // Export a different (empty) session to the same path.
    ExperimentSession s2;
    s2.start("S2");
    s2.end();
    QVERIFY(VnaCsvExporter::exportToFile(s2, path));

    const QStringList lines = readLines(path);
    // Empty session → header only → fewer lines than before.
    QCOMPARE(lines.size(), 1);
    QVERIFY(first_line_count > lines.size());
  }
};

QTEST_MAIN(TestVnaCsvExporter)
#include "test_vna_csv_exporter.moc"

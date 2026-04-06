/**
 * @file test_camera_frame_exporter.cpp
 * @brief Unit tests for mwa::analysis::CameraFrameExporter.
 */

#include <QtTest>
#include <QDir>
#include <QImage>
#include <QImageWriter>
#include <QTemporaryDir>

#include "analysis/camera_frame_exporter.h"
#include "analysis/experiment_session.h"

using namespace mwa::analysis;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

/// Create a solid-colour QImage of the given size.
static QImage makeImage(int width, int height, QRgb colour) {
  QImage img(width, height, QImage::Format_RGB32);
  img.fill(colour);
  return img;
}

/// Feed @p count camera frames into @p session (session must be active).
static void addFrames(ExperimentSession& session, int count) {
  for (int i = 0; i < count; ++i)
    session.addCameraFrame(makeImage(8, 8, qRgb(i * 10, 0, 0)));
}

// ---------------------------------------------------------------------------
// Test class
// ---------------------------------------------------------------------------

class TestCameraFrameExporter : public QObject {
  Q_OBJECT

 private slots:

  // -------------------------------------------------------------------------
  // Empty session: directory created, no files written, returns true
  // -------------------------------------------------------------------------

  void exportToDir_emptySession_createsDirAndReturnsTrue() {
    ExperimentSession session;
    session.start("Empty");
    session.end();

    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    const QString target = tmp.filePath("empty_export");

    QVERIFY(!QDir(target).exists());
    const bool ok = CameraFrameExporter::exportToDir(session, target);
    QVERIFY(ok);
    QVERIFY(CameraFrameExporter::lastError().isEmpty());
    QVERIFY(QDir(target).exists());
    QCOMPARE(QDir(target).entryList(QDir::Files).size(), 0);
  }

  // -------------------------------------------------------------------------
  // Single PNG frame: correct filename and readable image
  // -------------------------------------------------------------------------

  void exportToDir_singlePng_writesFrame0000() {
    ExperimentSession session;
    session.start("Single");
    const QImage original = makeImage(16, 16, qRgb(255, 0, 0));
    session.addCameraFrame(original);
    session.end();

    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    const QString target = tmp.filePath("single_png");

    QVERIFY(CameraFrameExporter::exportToDir(session, target, ImageFormat::kPng));
    QVERIFY(CameraFrameExporter::lastError().isEmpty());

    const QString expected_file = QDir(target).filePath("frame_0000.png");
    QVERIFY(QFile::exists(expected_file));

    const QImage loaded(expected_file);
    QVERIFY(!loaded.isNull());
    QCOMPARE(loaded.size(), original.size());
  }

  // -------------------------------------------------------------------------
  // Multiple PNG frames: correct count and sequential naming
  // -------------------------------------------------------------------------

  void exportToDir_multipleFrames_writesAllWithCorrectNames() {
    ExperimentSession session;
    session.start("Multi");
    addFrames(session, 5);
    session.end();

    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    const QString target = tmp.filePath("multi_png");

    QVERIFY(CameraFrameExporter::exportToDir(session, target, ImageFormat::kPng));

    const QDir dir(target);
    const QStringList files =
        dir.entryList(QStringList{QStringLiteral("*.png")},
                      QDir::Files, QDir::Name);
    QCOMPARE(files.size(), 5);
    QCOMPARE(files[0], QStringLiteral("frame_0000.png"));
    QCOMPARE(files[1], QStringLiteral("frame_0001.png"));
    QCOMPARE(files[4], QStringLiteral("frame_0004.png"));
  }

  // -------------------------------------------------------------------------
  // TIFF format: files have .tiff extension and no .png files appear
  // (skipped if the Qt TIFF image plugin is not available on this platform)
  // -------------------------------------------------------------------------

  void exportToDir_tiffFormat_writesTiffFiles() {
    if (!QImageWriter::supportedImageFormats().contains("tiff")) {
      QSKIP("Qt TIFF image plugin not available on this platform — skipping");
    }

    ExperimentSession session;
    session.start("Tiff");
    addFrames(session, 3);
    session.end();

    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    const QString target = tmp.filePath("tiff_export");

    QVERIFY(CameraFrameExporter::exportToDir(session, target, ImageFormat::kTiff));

    const QDir dir(target);
    const QStringList tiff_files =
        dir.entryList(QStringList{QStringLiteral("*.tiff")},
                      QDir::Files, QDir::Name);
    QCOMPARE(tiff_files.size(), 3);
    QCOMPARE(tiff_files[0], QStringLiteral("frame_0000.tiff"));
    QCOMPARE(tiff_files[2], QStringLiteral("frame_0002.tiff"));

    const QStringList png_files =
        dir.entryList(QStringList{QStringLiteral("*.png")}, QDir::Files);
    QCOMPARE(png_files.size(), 0);
  }

  // -------------------------------------------------------------------------
  // Zero-padding: indices stay 4+ digits wide
  // -------------------------------------------------------------------------

  void exportToDir_fourDigitPadding_correctForSmallIndices() {
    // Verify frame_0009.png (not frame_9.png)
    ExperimentSession session;
    session.start("Pad");
    addFrames(session, 10);
    session.end();

    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    const QString target = tmp.filePath("pad_export");

    QVERIFY(CameraFrameExporter::exportToDir(session, target, ImageFormat::kPng));

    const QString frame9 = QDir(target).filePath("frame_0009.png");
    QVERIFY(QFile::exists(frame9));
    // frame_9.png must NOT exist
    QVERIFY(!QFile::exists(QDir(target).filePath("frame_9.png")));
  }

  // -------------------------------------------------------------------------
  // Nested directory creation: missing parent dirs are created
  // -------------------------------------------------------------------------

  void exportToDir_nestedMissingDirs_createsAllParents() {
    ExperimentSession session;
    session.start("Nested");
    session.addCameraFrame(makeImage(4, 4, 0xFFFFFFFF));
    session.end();

    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    const QString target = tmp.filePath("a/b/c/frames");

    QVERIFY(!QDir(target).exists());
    QVERIFY(CameraFrameExporter::exportToDir(session, target, ImageFormat::kPng));
    QVERIFY(QDir(target).exists());
    QVERIFY(QFile::exists(QDir(target).filePath("frame_0000.png")));
  }

  // -------------------------------------------------------------------------
  // Image pixel fidelity (PNG lossless round-trip)
  // -------------------------------------------------------------------------

  void exportToDir_png_pixelContentSurvivesRoundTrip() {
    const QRgb kColour = qRgb(123, 45, 67);
    QImage original = makeImage(32, 32, kColour);

    ExperimentSession session;
    session.start("Fidelity");
    session.addCameraFrame(original);
    session.end();

    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    const QString target = tmp.filePath("fidelity");

    QVERIFY(CameraFrameExporter::exportToDir(session, target, ImageFormat::kPng));

    const QImage loaded(QDir(target).filePath("frame_0000.png"));
    QVERIFY(!loaded.isNull());
    // Check a sample pixel survives lossless round-trip.
    QCOMPARE(loaded.pixel(0, 0) & 0x00FFFFFF,
             original.pixel(0, 0) & 0x00FFFFFF);
  }

  // -------------------------------------------------------------------------
  // No files from inactive session (no frames recorded before start/after end)
  // -------------------------------------------------------------------------

  void exportToDir_inactiveSession_noFramesExported() {
    ExperimentSession session;
    // Never call start() — frames added here must be ignored.
    session.addCameraFrame(makeImage(8, 8, 0xFF000000));

    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    const QString target = tmp.filePath("inactive");

    QVERIFY(CameraFrameExporter::exportToDir(session, target, ImageFormat::kPng));
    QCOMPARE(QDir(target).entryList(QDir::Files).size(), 0);
  }

  // -------------------------------------------------------------------------
  // lastError() is empty on success
  // -------------------------------------------------------------------------

  void lastError_afterSuccess_isEmpty() {
    ExperimentSession session;
    session.start("Err");
    session.end();

    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());

    CameraFrameExporter::exportToDir(session, tmp.filePath("ok"));
    QVERIFY(CameraFrameExporter::lastError().isEmpty());
  }

  // -------------------------------------------------------------------------
  // Invalid directory path: returns false and sets lastError
  // -------------------------------------------------------------------------

  void exportToDir_invalidPath_returnsFalseAndSetsError() {
    ExperimentSession session;
    session.start("BadPath");
    session.addCameraFrame(makeImage(4, 4, 0xFFFFFFFF));
    session.end();

    // Use a path that cannot be created on macOS/Linux (null byte in name).
    // As an alternative, attempt to write inside a file (not a directory).
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());

    // Create a FILE at the target location so mkpath must fail.
    const QString blocker = tmp.filePath("blocked");
    {
      QFile f(blocker);
      QVERIFY(f.open(QIODevice::WriteOnly));
    }
    // Now try to export into "blocked/frames" — parent "blocked" is a file.
    const bool ok =
        CameraFrameExporter::exportToDir(session, blocker + "/frames");
    QVERIFY(!ok);
    QVERIFY(!CameraFrameExporter::lastError().isEmpty());
  }

  // -------------------------------------------------------------------------
  // Export to pre-existing directory (no error, no crash)
  // -------------------------------------------------------------------------

  void exportToDir_existingDir_succeeds() {
    ExperimentSession session;
    session.start("Exist");
    session.addCameraFrame(makeImage(4, 4, 0xFF00FF00));
    session.end();

    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    // tmp.path() already exists.
    QVERIFY(CameraFrameExporter::exportToDir(session, tmp.path(), ImageFormat::kPng));
    QVERIFY(QFile::exists(QDir(tmp.path()).filePath("frame_0000.png")));
  }

  // -------------------------------------------------------------------------
  // Default format argument is PNG
  // -------------------------------------------------------------------------

  void exportToDir_defaultFormat_writesPng() {
    ExperimentSession session;
    session.start("Default");
    session.addCameraFrame(makeImage(4, 4, 0xFF0000FF));
    session.end();

    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    const QString target = tmp.filePath("default_fmt");

    // Call without explicit format — should default to PNG.
    QVERIFY(CameraFrameExporter::exportToDir(session, target));
    QVERIFY(QFile::exists(QDir(target).filePath("frame_0000.png")));
    QVERIFY(!QFile::exists(QDir(target).filePath("frame_0000.tiff")));
  }
};

QTEST_GUILESS_MAIN(TestCameraFrameExporter)
#include "test_camera_frame_exporter.moc"

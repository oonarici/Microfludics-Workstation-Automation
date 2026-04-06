/**
 * @file camera_frame_exporter.cpp
 * @brief CameraFrameExporter implementation.
 * @author MWA Team
 * @date 2026-04-05
 *
 * @copyright LGPL-3.0-or-later
 */

#include "analysis/camera_frame_exporter.h"

#include <QDir>
#include <QString>

namespace mwa::analysis {

// ---------------------------------------------------------------------------
// Static member
// ---------------------------------------------------------------------------

QString CameraFrameExporter::last_error_;

// ---------------------------------------------------------------------------
// exportToDir()
// ---------------------------------------------------------------------------

bool CameraFrameExporter::exportToDir(const ExperimentSession& session,
                                       const QString& dir_path,
                                       ImageFormat format) {
  QDir dir(dir_path);
  if (!dir.mkpath(QStringLiteral("."))) {
    last_error_ =
        QStringLiteral("Cannot create directory: ") + dir_path;
    return false;
  }

  const bool is_tiff = (format == ImageFormat::kTiff);
  const char* const extension = is_tiff ? "tiff" : "png";
  const char* const qt_format = is_tiff ? "TIFF" : "PNG";

  const QVector<CameraFrame>& frames = session.cameraFrames();
  for (int i = 0; i < frames.size(); ++i) {
    const QString file_name =
        QStringLiteral("frame_%1.%2")
            .arg(i, 4, 10, QLatin1Char('0'))
            .arg(QLatin1String(extension));
    const QString file_path = dir.filePath(file_name);

    if (!frames[i].image.save(file_path, qt_format)) {
      last_error_ =
          QStringLiteral("Failed to write image: ") + file_path;
      return false;
    }
  }

  last_error_.clear();
  return true;
}

// ---------------------------------------------------------------------------
// lastError()
// ---------------------------------------------------------------------------

QString CameraFrameExporter::lastError() { return last_error_; }

}  // namespace mwa::analysis

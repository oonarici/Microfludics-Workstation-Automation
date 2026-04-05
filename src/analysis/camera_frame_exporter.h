/**
 * @file camera_frame_exporter.h
 * @brief Batch export of camera frames from an ExperimentSession to disk.
 * @author MWA Team
 * @date 2026-04-05
 *
 * Provides CameraFrameExporter, a stateless utility class that writes every
 * CameraFrame stored in an ExperimentSession to a directory on disk as
 * numbered image files.
 *
 * ### File-naming convention
 * Files are named @c frame_NNNN.png or @c frame_NNNN.tiff, where @c NNNN is
 * the zero-based frame index zero-padded to four digits:
 *
 * @code
 * frame_0000.png
 * frame_0001.png
 * ...
 * frame_0099.png
 * @endcode
 *
 * If the session contains more than 9 999 frames the index continues to grow
 * naturally (e.g. @c frame_10000.png) — the padding is a minimum width, not
 * a cap.
 *
 * ### Directory handling
 * The destination directory is created (including all missing parent
 * directories) if it does not already exist.  If creation fails,
 * exportToDir() returns @c false and lastError() describes the failure.
 *
 * ### Empty sessions
 * If the session contains no camera frames the directory is created (or
 * left as-is if it already exists) and exportToDir() returns @c true
 * without writing any files.
 *
 * @copyright LGPL-3.0-or-later
 */

#pragma once

#include <QString>

#include "analysis/experiment_session.h"

namespace mwa::analysis {

/**
 * @enum ImageFormat
 * @brief Image file format for CameraFrameExporter output.
 */
enum class ImageFormat {
  kPng,   ///< Portable Network Graphics (.png) — lossless, widely supported.
  kTiff,  ///< Tagged Image File Format (.tiff) — lossless, preferred for
          ///< scientific data.
};

/**
 * @class CameraFrameExporter
 * @brief Exports all camera frames from an ExperimentSession to a directory.
 *
 * All methods are static.  A static lastError() accessor returns a
 * human-readable message for the most recent failure.
 *
 * @note Not thread-safe: do not call exportToDir() concurrently from multiple
 *       threads without external synchronization.
 *
 * @see ExperimentSession
 * @see CameraFrame
 * @see ImageFormat
 */
class CameraFrameExporter {
 public:
  /**
   * @brief Write every camera frame in @p session to individual image files
   *        inside @p dir_path.
   *
   * Creates @p dir_path (and any missing parents) if it does not exist.
   * Writes one file per frame using the naming convention described in the
   * file header.  Overwrites any existing file with the same name.
   *
   * If any single frame fails to write, the method stops immediately,
   * records the error in lastError(), and returns @c false.  Frames already
   * written before the failure remain on disk.
   *
   * @param session  The session whose camera frames to export.
   * @param dir_path Path to the destination directory (absolute or relative).
   * @param format   Image format to use for all exported files.
   * @return @c true if all frames were written successfully (or the session
   *         has no frames); @c false on any I/O error.
   */
  static bool exportToDir(const ExperimentSession& session,
                           const QString& dir_path,
                           ImageFormat format = ImageFormat::kPng);

  /**
   * @brief Return a human-readable description of the last error.
   *
   * Returns an empty string if the last exportToDir() call succeeded.
   *
   * @return Error message string.
   */
  [[nodiscard]] static QString lastError();

 private:
  static QString last_error_;  ///< Most recent error description.
};

}  // namespace mwa::analysis

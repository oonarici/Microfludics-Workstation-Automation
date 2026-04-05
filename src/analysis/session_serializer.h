/**
 * @file session_serializer.h
 * @brief JSON serialization and deserialization for ExperimentSession.
 * @author MWA Team
 * @date 2026-04-05
 *
 * Provides SessionSerializer, a stateless utility class that saves an
 * ExperimentSession to a JSON file and restores one from a JSON file.
 *
 * ### JSON Format (format_version = 1)
 * The root object contains session metadata and one JSON array per device
 * type.  Camera frames are embedded as Base64-encoded PNG strings.
 * Timestamps are stored as ISO-8601 strings with millisecond precision
 * (Qt::ISODateWithMs).
 *
 * @copyright LGPL-3.0-or-later
 */

#pragma once

#include <QString>

#include "analysis/experiment_session.h"

namespace mwa::analysis {

/**
 * @class SessionSerializer
 * @brief Saves and loads an ExperimentSession to/from a JSON file.
 *
 * All methods are static.  A static @c lastError() accessor returns a
 * human-readable message for the most recent failure.
 *
 * @note Not thread-safe: do not call save() or load() concurrently from
 *       multiple threads without external synchronization.
 *
 * @see ExperimentSession
 */
class SessionSerializer {
 public:
  /**
   * @brief Serialize @p session to a JSON file at @p file_path.
   *
   * Overwrites any existing file.  Camera frames are encoded as Base64 PNG.
   * Timestamps are stored at millisecond resolution.
   *
   * @param session   The session to serialize (may be active or ended).
   * @param file_path Absolute or relative path to the destination file.
   * @return @c true on success; @c false if the file cannot be opened or
   *         written (lastError() describes the failure).
   */
  static bool save(const ExperimentSession& session,
                   const QString& file_path);

  /**
   * @brief Restore a session from a JSON file into @p session.
   *
   * Calls @c session.clear() before populating, so any previously held data
   * is discarded.  No signals are emitted on @p session during the restore.
   *
   * @param file_path Absolute or relative path to the source file.
   * @param session   Output parameter: receives the deserialized session.
   * @return @c true on success; @c false if the file cannot be opened,
   *         the JSON is malformed, or the format_version is unsupported
   *         (lastError() describes the failure).
   */
  static bool load(const QString& file_path, ExperimentSession& session);

  /**
   * @brief Return a human-readable description of the last error.
   *
   * Empty string if the last save() or load() succeeded.
   *
   * @return Error message string.
   */
  [[nodiscard]] static QString lastError();

 private:
  static QString last_error_;  ///< Most recent error description.
};

}  // namespace mwa::analysis

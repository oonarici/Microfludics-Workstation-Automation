/**
 * @file vna_csv_exporter.h
 * @brief CSV export for VNA measurements stored in an ExperimentSession.
 * @author MWA Team
 * @date 2026-04-05
 *
 * Provides VnaCsvExporter, a stateless utility class that writes all
 * VnaMeasurement entries from an ExperimentSession to a single CSV file.
 *
 * ### CSV Format
 * The first line is a fixed header row.  Each subsequent line represents one
 * data point from one sweep:
 *
 * @code
 * sweep_index,timestamp,start_frequency_hz,stop_frequency_hz,frequency_hz,magnitude_db
 * 0,2026-04-05T10:00:00.000,1000000,10000000,1000000,-12.5
 * @endcode
 *
 * - @c sweep_index: zero-based index of the sweep within the session.
 * - @c timestamp: ISO-8601 with millisecond precision (Qt::ISODateWithMs).
 * - @c start_frequency_hz / @c stop_frequency_hz: sweep range (Hz).
 * - @c frequency_hz: excitation frequency of this point (Hz).
 * - @c magnitude_db: S-parameter magnitude at this point (dB).
 *
 * If the session contains no VNA measurements the file is created with only
 * the header row and exportToFile() returns @c true.
 *
 * @copyright LGPL-3.0-or-later
 */

#pragma once

#include <QString>

#include "analysis/experiment_session.h"

namespace mwa::analysis {

/**
 * @class VnaCsvExporter
 * @brief Exports all VNA measurements from an ExperimentSession to a CSV file.
 *
 * All methods are static.  A static @c lastError() accessor returns a
 * human-readable message for the most recent failure.
 *
 * @note Not thread-safe: do not call exportToFile() concurrently from multiple
 *       threads without external synchronization.
 *
 * @see ExperimentSession
 * @see VnaMeasurement
 */
class VnaCsvExporter {
 public:
  /**
   * @brief Write all VNA measurements from @p session to a CSV file.
   *
   * Overwrites any existing file at @p file_path.  Produces a header row
   * followed by one data row per sweep point across all sweeps.  If the
   * session contains no VNA measurements, only the header row is written and
   * the method returns @c true.
   *
   * @param session   The session whose VNA measurements to export.
   * @param file_path Absolute or relative path to the destination CSV file.
   * @return @c true on success; @c false if the file cannot be opened or
   *         written (lastError() describes the failure).
   */
  static bool exportToFile(const ExperimentSession& session,
                            const QString& file_path);

  /**
   * @brief Return a human-readable description of the last error.
   *
   * Empty string if the last exportToFile() call succeeded.
   *
   * @return Error message string.
   */
  [[nodiscard]] static QString lastError();

 private:
  static QString last_error_;  ///< Most recent error description.
};

}  // namespace mwa::analysis

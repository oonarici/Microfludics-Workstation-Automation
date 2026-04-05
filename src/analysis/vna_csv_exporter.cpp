/**
 * @file vna_csv_exporter.cpp
 * @brief VnaCsvExporter implementation.
 * @author MWA Team
 * @date 2026-04-05
 *
 * @copyright LGPL-3.0-or-later
 */

#include "analysis/vna_csv_exporter.h"

#include <QFile>
#include <QTextStream>

namespace mwa::analysis {

// ---------------------------------------------------------------------------
// Static member
// ---------------------------------------------------------------------------

QString VnaCsvExporter::last_error_;

// ---------------------------------------------------------------------------
// exportToFile()
// ---------------------------------------------------------------------------

bool VnaCsvExporter::exportToFile(const ExperimentSession& session,
                                   const QString& file_path) {
  QFile file(file_path);
  if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate |
                 QIODevice::Text)) {
    last_error_ =
        QStringLiteral("Cannot open file for writing: ") + file_path;
    return false;
  }

  QTextStream out(&file);

  out << QStringLiteral(
      "sweep_index,timestamp,start_frequency_hz,stop_frequency_hz,"
      "frequency_hz,magnitude_db\n");

  const QVector<VnaMeasurement>& measurements = session.vnaMeasurements();
  for (int sweep_idx = 0; sweep_idx < measurements.size(); ++sweep_idx) {
    const VnaMeasurement& m = measurements[sweep_idx];
    const QString ts = m.timestamp.isValid()
                           ? m.timestamp.toString(Qt::ISODateWithMs)
                           : QString{};
    const QString start_freq = QString::number(m.start_frequency, 'f', 0);
    const QString stop_freq  = QString::number(m.stop_frequency, 'f', 0);

    for (int i = 0; i < m.frequencies.size(); ++i) {
      out << sweep_idx << ','
          << ts << ','
          << start_freq << ','
          << stop_freq << ','
          << QString::number(m.frequencies[i], 'f', 6) << ','
          << QString::number(m.magnitudes[i], 'f', 6) << '\n';
    }
  }

  if (out.status() != QTextStream::Ok) {
    last_error_ =
        QStringLiteral("Write failed (disk full?): ") + file_path;
    return false;
  }

  last_error_.clear();
  return true;
}

// ---------------------------------------------------------------------------
// lastError()
// ---------------------------------------------------------------------------

QString VnaCsvExporter::lastError() { return last_error_; }

}  // namespace mwa::analysis

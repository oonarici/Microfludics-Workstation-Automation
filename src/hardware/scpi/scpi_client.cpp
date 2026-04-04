/**
 * @file scpi_client.cpp
 * @brief ScpiClient implementation — SCPI command formatting and
 *        response parsing.
 * @author MWA Team
 * @date 2026-04-02
 *
 * @copyright LGPL-3.0-or-later
 */

#include "hardware/scpi/scpi_client.h"

#include <QByteArray>
#include <QString>
#include <QStringList>

#include <utility>

namespace mwa::hardware {

// -----------------------------------------------------------------
// Construction / destruction / move
// -----------------------------------------------------------------

ScpiClient::ScpiClient(
    std::unique_ptr<ScpiTransport> transport)
    : transport_(std::move(transport)),
      terminator_("\n") {}

ScpiClient::~ScpiClient() {
  close();
}

ScpiClient::ScpiClient(ScpiClient&&) noexcept = default;

ScpiClient& ScpiClient::operator=(ScpiClient&&) noexcept =
    default;

// -----------------------------------------------------------------
// Connection management
// -----------------------------------------------------------------

bool ScpiClient::open(const QString& resource) {
  if (!transport_) {
    return false;
  }
  return transport_->open(resource);
}

void ScpiClient::close() {
  if (transport_) {
    transport_->close();
  }
}

bool ScpiClient::isOpen() const {
  return transport_ && transport_->isOpen();
}

// -----------------------------------------------------------------
// SCPI I/O
// -----------------------------------------------------------------

QString ScpiClient::query(const QString& cmd,
                          int timeout_ms) {
  if (!sendRaw(cmd)) {
    return {};
  }
  return readResponse(timeout_ms);
}

bool ScpiClient::command(const QString& cmd,
                         int timeout_ms) {
  if (!sendRaw(cmd)) {
    return false;
  }

  // Ask the instrument to signal completion.
  if (!sendRaw(QStringLiteral("*OPC?"))) {
    return false;
  }

  QString response = readResponse(timeout_ms);
  return response.trimmed() == QStringLiteral("1");
}

// -----------------------------------------------------------------
// Common IEEE 488.2 commands
// -----------------------------------------------------------------

QString ScpiClient::identify() {
  return query(QStringLiteral("*IDN?"));
}

void ScpiClient::reset() {
  sendRaw(QStringLiteral("*RST"));
}

QString ScpiClient::lastError() {
  return query(QStringLiteral("SYST:ERR?"));
}

bool ScpiClient::checkErrors() {
  for (int i = 0; i < kMaxErrorPolls; ++i) {
    QString err =
        query(QStringLiteral("SYST:ERR?"));

    if (err.isEmpty()) {
      // Communication failure — assume error state.
      return false;
    }

    QString trimmed = err.trimmed();
    if (trimmed.startsWith(QStringLiteral("0,")) ||
        trimmed.startsWith(QStringLiteral("+0,"))) {
      return (i == 0);  // true only if first query was clean
    }
  }

  // Exhausted poll limit — assume errors remain.
  return false;
}

// -----------------------------------------------------------------
// Typed response parsers
// -----------------------------------------------------------------

QVector<double> ScpiClient::queryDoubleList(
    const QString& cmd, int timeout_ms) {
  QString response = query(cmd, timeout_ms);
  if (response.isEmpty()) {
    return {};
  }

  QStringList parts = response.split(
      QLatin1Char(','), Qt::SkipEmptyParts);
  QVector<double> values;
  values.reserve(parts.size());

  for (const QString& part : parts) {
    bool ok = false;
    double val = part.trimmed().toDouble(&ok);
    if (!ok) {
      return {};  // Abort on any parse failure.
    }
    values.append(val);
  }

  return values;
}

QByteArray ScpiClient::queryBlockData(
    const QString& cmd, int timeout_ms) {
  if (!sendRaw(cmd)) {
    return {};
  }

  // Read the entire raw response.
  if (!transport_) {
    return {};
  }
  QByteArray raw = transport_->read(timeout_ms);
  if (raw.isEmpty()) {
    return {};
  }

  // Parse IEEE 488.2 definite-length block: #<d><n><data>
  // Find the '#' header character.
  int hash_pos = raw.indexOf('#');
  if (hash_pos < 0 ||
      hash_pos + 1 >= raw.size()) {
    return {};
  }

  // <d> is one ASCII digit giving the number of length digits.
  char d_char = raw.at(hash_pos + 1);
  int d = d_char - '0';
  if (d < 1 || d > 9) {
    return {};
  }

  int n_start = hash_pos + 2;
  if (n_start + d > raw.size()) {
    return {};
  }

  // <n> is a d-digit ASCII number giving the payload byte count.
  QByteArray n_bytes = raw.mid(n_start, d);
  bool ok = false;
  int payload_len = n_bytes.toInt(&ok);
  if (!ok || payload_len < 0) {
    return {};
  }

  int data_start = n_start + d;
  if (data_start + payload_len > raw.size()) {
    return {};  // Incomplete data.
  }

  return raw.mid(data_start, payload_len);
}

// -----------------------------------------------------------------
// Configuration
// -----------------------------------------------------------------

void ScpiClient::setTerminator(const QByteArray& term) {
  terminator_ = term;
}

// -----------------------------------------------------------------
// Private helpers
// -----------------------------------------------------------------

bool ScpiClient::sendRaw(const QString& cmd) {
  if (!transport_ || !transport_->isOpen()) {
    return false;
  }
  QByteArray data = cmd.toUtf8() + terminator_;
  return transport_->write(data);
}

QString ScpiClient::readResponse(int timeout_ms) {
  if (!transport_) {
    return {};
  }
  QByteArray raw = transport_->read(timeout_ms);
  return QString::fromUtf8(raw).trimmed();
}

}  // namespace mwa::hardware

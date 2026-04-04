/**
 * @file serial_transport.cpp
 * @brief SerialTransport implementation — QSerialPort blocking I/O.
 * @author MWA Team
 * @date 2026-04-02
 *
 * @copyright LGPL-3.0-or-later
 */

#ifdef MWA_HAS_SERIAL_PORT

#include "hardware/scpi/serial_transport.h"

#include <QElapsedTimer>

namespace mwa::hardware {

SerialTransport::SerialTransport()
    : port_(std::make_unique<QSerialPort>()),
      baud_rate_(9600),
      terminator_("\n") {}

SerialTransport::~SerialTransport() {
  close();
}

void SerialTransport::setBaudRate(qint32 baud) {
  baud_rate_ = baud;
}

void SerialTransport::setTerminator(const QByteArray& terminator) {
  terminator_ = terminator;
}

bool SerialTransport::open(const QString& resource) {
  if (port_->isOpen()) {
    port_->close();
  }

  port_->setPortName(resource);
  port_->setBaudRate(baud_rate_);
  port_->setDataBits(QSerialPort::Data8);
  port_->setParity(QSerialPort::NoParity);
  port_->setStopBits(QSerialPort::OneStop);
  port_->setFlowControl(QSerialPort::NoFlowControl);

  if (!port_->open(QIODevice::ReadWrite)) {
    return false;
  }

  // Flush any stale data sitting in the buffer.
  port_->clear();
  return true;
}

void SerialTransport::close() {
  if (port_ && port_->isOpen()) {
    port_->close();
  }
}

bool SerialTransport::isOpen() const {
  return port_ && port_->isOpen();
}

bool SerialTransport::write(const QByteArray& data) {
  if (!isOpen()) {
    return false;
  }

  qint64 written = port_->write(data);
  if (written != data.size()) {
    return false;
  }

  return port_->waitForBytesWritten(kWriteTimeoutMs);
}

QByteArray SerialTransport::read(int timeout_ms) {
  if (!isOpen()) {
    return {};
  }

  QByteArray buffer;
  QElapsedTimer timer;
  timer.start();

  while (!timer.hasExpired(timeout_ms)) {
    int remaining =
        timeout_ms - static_cast<int>(timer.elapsed());
    if (remaining <= 0) {
      break;
    }

    if (port_->waitForReadyRead(remaining)) {
      buffer.append(port_->readAll());

      if (buffer.contains(terminator_)) {
        return buffer;
      }
    }
  }

  // Return whatever was accumulated (may be empty).
  return buffer;
}

}  // namespace mwa::hardware

#endif  // MWA_HAS_SERIAL_PORT

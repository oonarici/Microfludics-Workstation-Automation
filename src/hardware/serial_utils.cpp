/**
 * @file serial_utils.cpp
 * @brief Shared serial I/O utility implementations.
 * @author MWA Team
 * @date 2026-03-25
 *
 * Implements the free functions declared in serial_utils.h.
 *
 * @copyright LGPL-3.0-or-later
 */

#include "hardware/serial_utils.h"

#include <QIODevice>
#include <QSerialPort>

namespace mwa::hardware::serial_utils {

QString sendCommand(QSerialPort* port, const QString& cmd,
                    int response_wait_ms) {
  if (!port || !port->isOpen()) {
    return {};
  }

  const QByteArray data = (cmd + QStringLiteral("\r\n")).toUtf8();
  port->write(data);

  if (!port->waitForBytesWritten(response_wait_ms)) {
    return {};
  }

  QByteArray response;
  while (port->waitForReadyRead(response_wait_ms)) {
    response.append(port->readAll());
    if (response.contains('\n')) {
      break;
    }
  }

  // Isolate the first line — discard anything beyond the first \n.
  const int newline_pos = response.indexOf('\n');
  if (newline_pos >= 0) {
    response.truncate(newline_pos);
  }

  return QString::fromUtf8(response).trimmed();
}

bool openPort(QSerialPort* port, const QString& name, qint32 baud) {
  if (!port) {
    return false;
  }

  port->setPortName(name);
  port->setBaudRate(baud);
  port->setDataBits(QSerialPort::Data8);
  port->setParity(QSerialPort::NoParity);
  port->setStopBits(QSerialPort::OneStop);
  port->setFlowControl(QSerialPort::NoFlowControl);

  if (!port->open(QIODevice::ReadWrite)) {
    return false;
  }

  port->clear();
  return true;
}

}  // namespace mwa::hardware::serial_utils

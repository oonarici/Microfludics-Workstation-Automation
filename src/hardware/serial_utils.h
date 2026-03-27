/**
 * @file serial_utils.h
 * @brief Shared utility functions for serial-port-based device drivers.
 * @author MWA Team
 * @date 2026-03-25
 *
 * Provides common serial I/O helpers used by all hardware drivers that
 * communicate over QSerialPort.  These free functions are designed to
 * be called from within a CommandQueue command (i.e. on the worker
 * thread that owns the QSerialPort).
 *
 * @copyright LGPL-3.0-or-later
 */

#pragma once

#include <QString>

class QSerialPort;

namespace mwa::hardware::serial_utils {

/**
 * @brief Send an ASCII command and read the first response line.
 *
 * Writes @p cmd + "\\r\\n" to @p port using blocking I/O, then waits
 * for a complete line (terminated by '\\n').  Only the first line is
 * returned; any excess bytes beyond the first line terminator are
 * discarded from the port buffer.
 *
 * @param port           Open QSerialPort to communicate with.
 * @param cmd            Command string (without terminator).
 * @param response_wait_ms  Timeout in milliseconds for both the write
 *                          and each read wait cycle.
 * @return The trimmed first response line, or an empty string on
 *         timeout or if the port is null / not open.
 *
 * @note Must be called from the thread that owns @p port.
 */
QString sendCommand(QSerialPort* port, const QString& cmd,
                    int response_wait_ms);

/**
 * @brief Configure and open a serial port with 8N1 settings.
 *
 * Sets data bits to 8, no parity, one stop bit, no flow control,
 * then opens the port in read-write mode and flushes stale data.
 *
 * @param port  The QSerialPort to configure and open.
 * @param name  Platform-specific port identifier (e.g. "COM3").
 * @param baud  Baud rate in bits per second.
 * @return @c true if the port was opened successfully.
 *
 * @note Must be called from the thread that owns @p port.
 */
bool openPort(QSerialPort* port, const QString& name, qint32 baud);

/**
 * @brief Test whether a serial response indicates success.
 *
 * @param response The trimmed response string from sendCommand().
 * @return @c true if @p response starts with "OK" (case-insensitive).
 */
[[nodiscard]] inline bool isOkResponse(const QString& response) {
  return response.startsWith(QStringLiteral("OK"), Qt::CaseInsensitive);
}

/**
 * @brief Format a human-readable error message for a failed command.
 *
 * Produces a string of the form
 * "<driver_name>: <action> failed — <detail>" where detail is either
 * the device's response or "timeout" if the response was empty.
 *
 * @param driver_name  Display name of the driver class (e.g. "LedController").
 * @param action       Short description of the failed operation.
 * @param response     The raw response from sendCommand() (may be empty).
 * @return Formatted error string.
 */
[[nodiscard]] inline QString formatCommandError(
    const QString& driver_name, const QString& action,
    const QString& response) {
  return QStringLiteral("%1: %2 failed — %3")
      .arg(driver_name, action,
           response.isEmpty() ? QStringLiteral("timeout") : response);
}

}  // namespace mwa::hardware::serial_utils

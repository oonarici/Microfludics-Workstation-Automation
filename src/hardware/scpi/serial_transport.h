/**
 * @file serial_transport.h
 * @brief ScpiTransport implementation over QSerialPort.
 * @author MWA Team
 * @date 2026-04-02
 *
 * Provides SerialTransport, a concrete ScpiTransport that wraps
 * QSerialPort for serial / USB-serial instrument communication.
 * This file is conditionally compiled only when Qt6::SerialPort
 * is available (guarded by MWA_HAS_SERIAL_PORT).
 *
 * @copyright LGPL-3.0-or-later
 */

#pragma once

#ifdef MWA_HAS_SERIAL_PORT

#include <QByteArray>
#include <QSerialPort>
#include <QString>

#include <memory>

#include "hardware/scpi/scpi_transport.h"

namespace mwa::hardware {

/**
 * @class SerialTransport
 * @brief ScpiTransport over a QSerialPort link (8N1).
 *
 * Wraps QSerialPort with blocking I/O suitable for use on a
 * CommandQueue worker thread.  The port is opened with 8-N-1
 * settings and no flow control.
 *
 * The baud rate and line terminator are configurable before
 * calling open().
 */
class SerialTransport : public ScpiTransport {
 public:
  /**
   * @brief Construct a SerialTransport with default settings.
   *
   * Default baud rate is 9600 and default terminator is "\\n".
   */
  SerialTransport();

  /** @brief Destructor — closes the port if still open. */
  ~SerialTransport() override;

  // Non-copyable, non-movable (owns QSerialPort).
  SerialTransport(const SerialTransport&) = delete;
  SerialTransport& operator=(const SerialTransport&) = delete;
  SerialTransport(SerialTransport&&) = delete;
  SerialTransport& operator=(SerialTransport&&) = delete;

  /**
   * @brief Set the baud rate used when open() is called.
   *
   * Must be called before open().  Calling after open() has no
   * effect until the port is re-opened.
   *
   * @param baud  Baud rate in bits per second (e.g. 9600, 115200).
   */
  void setBaudRate(qint32 baud);

  /**
   * @brief Set the line terminator used to detect end-of-response.
   *
   * @param terminator  Byte sequence that marks end of a response
   *                    line (default "\\n").
   */
  void setTerminator(const QByteArray& terminator);

  /**
   * @brief Open the serial port with 8N1 settings.
   *
   * @param resource  Port name (e.g. "COM3", "/dev/ttyUSB0").
   * @return @c true if the port was opened successfully.
   */
  bool open(const QString& resource) override;

  /**
   * @brief Close the serial port.
   *
   * Safe to call when already closed.
   */
  void close() override;

  /**
   * @brief Query whether the serial port is open.
   *
   * @return @c true if the port is open.
   */
  [[nodiscard]] bool isOpen() const override;

  /**
   * @brief Write raw bytes to the serial port (blocking).
   *
   * @param data  Bytes to send.
   * @return @c true if all bytes were written within the internal
   *         write timeout (5 000 ms).
   */
  bool write(const QByteArray& data) override;

  /**
   * @brief Read bytes until the terminator is found or timeout.
   *
   * @param timeout_ms  Maximum time to wait in milliseconds.
   * @return The accumulated bytes including the terminator, or an
   *         empty QByteArray on timeout / error.
   */
  QByteArray read(int timeout_ms) override;

 private:
  /// @brief Internal write-completion timeout in milliseconds.
  static constexpr int kWriteTimeoutMs = 5000;

  std::unique_ptr<QSerialPort> port_;  ///< Underlying serial port.
  qint32 baud_rate_;                   ///< Configured baud rate.
  QByteArray terminator_;              ///< Response line terminator.
};

}  // namespace mwa::hardware

#endif  // MWA_HAS_SERIAL_PORT

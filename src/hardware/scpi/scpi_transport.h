/**
 * @file scpi_transport.h
 * @brief Abstract transport interface for SCPI instrument
 *        communication.
 * @author MWA Team
 * @date 2026-04-02
 *
 * Defines the ScpiTransport abstract base class that decouples
 * ScpiClient from any specific physical layer (serial, TCP, USB-TMC,
 * etc.).  Concrete implementations provide the actual I/O.
 *
 * @copyright LGPL-3.0-or-later
 */

#pragma once

#include <QByteArray>
#include <QString>

namespace mwa::hardware {

/**
 * @class ScpiTransport
 * @brief Abstract transport layer for SCPI instrument I/O.
 *
 * A ScpiTransport encapsulates the raw byte-level communication
 * with a single instrument.  Implementations are expected to use
 * blocking I/O because they run on a dedicated CommandQueue worker
 * thread.
 *
 * This is a plain abstract class (not a QObject) so that it can
 * be composed freely via std::unique_ptr.
 */
class ScpiTransport {
 public:
  /** @brief Virtual destructor for safe polymorphic deletion. */
  virtual ~ScpiTransport() = default;

  /**
   * @brief Open the transport to the given resource.
   *
   * @param resource  Resource identifier whose format is defined by
   *                  the concrete implementation (e.g. a serial port
   *                  name such as "COM3" or "/dev/ttyUSB0").
   * @return @c true if the transport was opened successfully.
   */
  virtual bool open(const QString& resource) = 0;

  /**
   * @brief Close the transport, releasing the underlying resource.
   *
   * Calling close() on an already-closed transport is a no-op.
   */
  virtual void close() = 0;

  /**
   * @brief Query whether the transport is currently open.
   *
   * @return @c true if the transport is open and ready for I/O.
   */
  [[nodiscard]] virtual bool isOpen() const = 0;

  /**
   * @brief Write raw bytes to the instrument.
   *
   * The caller is responsible for appending any required line
   * terminator before calling this method.
   *
   * @param data  Bytes to send.
   * @return @c true if all bytes were written successfully.
   */
  virtual bool write(const QByteArray& data) = 0;

  /**
   * @brief Read bytes from the instrument until a terminator is
   *        found or the timeout elapses.
   *
   * @param timeout_ms  Maximum time to wait in milliseconds.
   * @return The bytes read (may be empty on timeout or error).
   */
  virtual QByteArray read(int timeout_ms) = 0;
};

}  // namespace mwa::hardware

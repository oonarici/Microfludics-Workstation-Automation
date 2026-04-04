/**
 * @file scpi_client.h
 * @brief Reusable SCPI command client built on ScpiTransport.
 * @author MWA Team
 * @date 2026-04-02
 *
 * Provides ScpiClient, a thin convenience layer that formats SCPI
 * commands, sends them through a pluggable ScpiTransport, and
 * parses common response formats (strings, numeric lists, IEEE 488.2
 * block data).
 *
 * ScpiClient is a plain C++ class (not a QObject).  It is designed
 * to be instantiated once per instrument and used exclusively from
 * a single CommandQueue worker thread, making external locking
 * unnecessary.
 *
 * @copyright LGPL-3.0-or-later
 */

#pragma once

#include <QByteArray>
#include <QString>
#include <QVector>

#include <memory>

#include "hardware/scpi/scpi_transport.h"

namespace mwa::hardware {

/**
 * @class ScpiClient
 * @brief High-level SCPI command interface over a ScpiTransport.
 *
 * ScpiClient owns a ScpiTransport and exposes typed helpers for the
 * most common SCPI interaction patterns:
 *
 * - query()            — send a command, read the response
 * - command()          — send a command, optionally wait for *OPC?
 * - identify()         — *IDN?
 * - reset()            — *RST
 * - lastError()        — SYST:ERR?
 * - checkErrors()      — poll SYST:ERR? until no errors remain
 * - queryDoubleList()  — parse comma-separated numeric responses
 * - queryBlockData()   — read IEEE 488.2 definite-length block data
 *
 * Thread safety: one ScpiClient instance must be used from one
 * thread only (the CommandQueue worker thread for the device).
 */
class ScpiClient {
 public:
  /// @brief Default command timeout in milliseconds.
  static constexpr int kDefaultTimeoutMs = 1000;

  /// @brief Default timeout for numeric-list queries (ms).
  static constexpr int kListQueryTimeoutMs = 5000;

  /// @brief Default timeout for block-data queries (ms).
  static constexpr int kBlockQueryTimeoutMs = 10000;

  /**
   * @brief Construct a ScpiClient that owns the given transport.
   *
   * @param transport  Transport implementation (must not be null).
   *                   Ownership is transferred to the ScpiClient.
   */
  explicit ScpiClient(
      std::unique_ptr<ScpiTransport> transport);

  /** @brief Destructor — closes the transport if still open. */
  ~ScpiClient();

  // Non-copyable (unique_ptr member).
  ScpiClient(const ScpiClient&) = delete;
  ScpiClient& operator=(const ScpiClient&) = delete;

  // Movable.
  ScpiClient(ScpiClient&&) noexcept;
  ScpiClient& operator=(ScpiClient&&) noexcept;

  // -- Connection management ----------------------------------------

  /**
   * @brief Open the underlying transport to @p resource.
   *
   * @param resource  Resource string forwarded to the transport
   *                  (e.g. a serial port name or IP address).
   * @return @c true if the transport was opened successfully.
   */
  bool open(const QString& resource);

  /**
   * @brief Close the underlying transport.
   *
   * Safe to call when already closed.
   */
  void close();

  /**
   * @brief Query whether the transport is open.
   *
   * @return @c true if the transport is open and ready.
   */
  [[nodiscard]] bool isOpen() const;

  // -- SCPI I/O -----------------------------------------------------

  /**
   * @brief Send a SCPI query and return the trimmed response.
   *
   * Appends the configured terminator to @p cmd, writes it to the
   * transport, reads the response, and returns it trimmed of
   * leading / trailing whitespace and terminators.
   *
   * @param cmd         SCPI command string (without terminator).
   * @param timeout_ms  Response timeout in milliseconds.
   * @return The trimmed response, or an empty string on timeout.
   */
  QString query(const QString& cmd,
                int timeout_ms = kDefaultTimeoutMs);

  /**
   * @brief Send a SCPI command that produces no data response.
   *
   * Appends the configured terminator and writes @p cmd.  Then
   * sends "*OPC?" and waits for the instrument to reply "1",
   * confirming the operation completed.
   *
   * @param cmd         SCPI command string (without terminator).
   * @param timeout_ms  Timeout for the *OPC? confirmation.
   * @return @c true if *OPC? returned "1" within the timeout.
   */
  bool command(const QString& cmd,
               int timeout_ms = kDefaultTimeoutMs);

  // -- Common IEEE 488.2 commands -----------------------------------

  /**
   * @brief Send "*IDN?" and return the instrument identity.
   *
   * @return Identity string, or empty on failure.
   */
  QString identify();

  /**
   * @brief Send "*RST" to reset the instrument.
   */
  void reset();

  /**
   * @brief Send "SYST:ERR?" and return the error string.
   *
   * @return The raw error string returned by the instrument.
   */
  QString lastError();

  /**
   * @brief Poll "SYST:ERR?" until no errors remain.
   *
   * Repeatedly queries the error queue until the response starts
   * with "0," or "+0," (the standard "no error" prefix).
   *
   * @return @c true if no errors were found on the first query.
   */
  bool checkErrors();

  // -- Typed response parsers ---------------------------------------

  /**
   * @brief Send a query and parse a comma-separated list of doubles.
   *
   * @param cmd         SCPI query command.
   * @param timeout_ms  Response timeout in milliseconds.
   * @return Vector of parsed double values (empty on parse error
   *         or timeout).
   */
  QVector<double> queryDoubleList(
      const QString& cmd,
      int timeout_ms = kListQueryTimeoutMs);

  /**
   * @brief Send a query and read IEEE 488.2 definite-length block
   *        data.
   *
   * Reads a response of the form @c #\<d\>\<n\>\<data\> where
   * @c d is one digit giving the number of length digits and @c n
   * is the byte count of the payload.
   *
   * @param cmd         SCPI query command.
   * @param timeout_ms  Response timeout in milliseconds.
   * @return The raw block payload, or empty on error / timeout.
   */
  QByteArray queryBlockData(
      const QString& cmd,
      int timeout_ms = kBlockQueryTimeoutMs);

  // -- Configuration ------------------------------------------------

  /**
   * @brief Set the command terminator appended to every command.
   *
   * @param term  Terminator byte sequence (default "\\n").
   */
  void setTerminator(const QByteArray& term);

 private:
  /**
   * @brief Send raw bytes (cmd + terminator) through the transport.
   *
   * @param cmd  Command text (terminator is appended automatically).
   * @return @c true if the write succeeded.
   */
  bool sendRaw(const QString& cmd);

  /**
   * @brief Read a response and return it as a trimmed QString.
   *
   * @param timeout_ms  Read timeout in milliseconds.
   * @return Trimmed response string, or empty on timeout.
   */
  QString readResponse(int timeout_ms);

  /// @brief Maximum number of error-queue polls in checkErrors().
  static constexpr int kMaxErrorPolls = 50;

  std::unique_ptr<ScpiTransport> transport_;  ///< Owned transport.
  QByteArray terminator_;  ///< Command line terminator.
};

}  // namespace mwa::hardware

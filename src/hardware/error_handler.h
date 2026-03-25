/**
 * @file error_handler.h
 * @brief Error recovery framework for hardware device communication.
 * @author MWA Team
 * @date 2026-03-25
 *
 * Provides configurable retry logic with exponential backoff, timeout
 * handling, and automatic reconnection for DeviceInterface instances.
 *
 * @copyright LGPL-3.0-or-later
 */

#pragma once

#include <QObject>
#include <QTimer>

#include <functional>

#include "hardware/device_interface.h"

namespace mwa::hardware {

/**
 * @struct RetryPolicy
 * @brief Configuration for the retry behaviour of an ErrorHandler.
 */
struct RetryPolicy {
  int max_attempts = 3;       ///< Maximum number of attempts (including
                               ///< the initial attempt).
  int base_delay_ms = 500;    ///< Base delay between retries in
                               ///< milliseconds.
  double backoff_factor = 2.0; ///< Multiplier applied to the delay on
                               ///< each successive retry.
  int max_delay_ms = 10000;   ///< Upper bound on the inter-retry delay.
};

/**
 * @class ErrorHandler
 * @brief Retry, timeout, and auto-reconnect logic for device operations.
 *
 * ErrorHandler wraps a callable device operation and re-executes it
 * according to a configurable RetryPolicy if it fails. Each attempt
 * may be guarded by a timeout. When all retries are exhausted the
 * allRetriesFailed() signal is emitted.
 *
 * Optionally, ErrorHandler can monitor a DeviceInterface for unexpected
 * disconnection and automatically attempt reconnection.
 *
 * All retry/reconnect activity is logged through the core Logger.
 *
 * @note ErrorHandler does not run on a separate thread. It uses QTimer
 *       for delays and timeouts, so the caller's event loop must be
 *       running.
 *
 * @see RetryPolicy
 * @see DeviceInterface
 */
class ErrorHandler : public QObject {
  Q_OBJECT

 public:
  /**
   * @brief Construct an ErrorHandler.
   *
   * @param parent Optional QObject parent for Qt ownership management.
   */
  explicit ErrorHandler(QObject* parent = nullptr);

  /**
   * @brief Destructor — stops any active retry or reconnect timers.
   */
  ~ErrorHandler() override;

  // Non-copyable, non-movable (QObject restriction).
  ErrorHandler(const ErrorHandler&) = delete;
  ErrorHandler& operator=(const ErrorHandler&) = delete;
  ErrorHandler(ErrorHandler&&) = delete;
  ErrorHandler& operator=(ErrorHandler&&) = delete;

  /**
   * @brief Set the retry policy for subsequent execute() calls.
   *
   * @param policy The RetryPolicy to apply.
   */
  void setRetryPolicy(const RetryPolicy& policy);

  /**
   * @brief Return the current retry policy.
   *
   * @return A const reference to the active RetryPolicy.
   */
  [[nodiscard]] const RetryPolicy& retryPolicy() const;

  /**
   * @brief Execute an operation with retry and optional timeout.
   *
   * The @p operation callable is invoked immediately. If it returns
   * @c false (indicating failure), the ErrorHandler waits according to
   * the current RetryPolicy and retries. On success (returns @c true)
   * the operationSucceeded() signal is emitted.
   *
   * If a @p timeout_ms is specified and the operation does not call
   * back within that period the attempt is treated as a failure.
   *
   * @param operation  Callable returning @c true on success, @c false
   *                   on failure.
   * @param timeout_ms Per-attempt timeout in milliseconds. 0 means no
   *                   timeout (the default).
   */
  void execute(std::function<bool()> operation, int timeout_ms = 0);

  /**
   * @brief Cancel any in-progress retry sequence.
   *
   * Stops the retry timer. Does not affect auto-reconnect monitoring.
   */
  void cancel();

  /**
   * @brief Query whether a retry sequence is currently in progress.
   *
   * @return @c true if an execute() call is actively retrying.
   */
  [[nodiscard]] bool isRetrying() const;

  /**
   * @brief Return the current attempt number (1-based) during a retry
   *        sequence.
   *
   * @return The current attempt number, or 0 if not retrying.
   */
  [[nodiscard]] int currentAttempt() const;

  /**
   * @brief Begin monitoring a device for unexpected disconnection.
   *
   * If the device transitions to DeviceState::kDisconnected or
   * DeviceState::kError while being monitored, ErrorHandler will
   * automatically attempt to reconnect using the current retry policy.
   *
   * @param device Non-null pointer to the device to monitor.
   */
  void monitorDevice(DeviceInterface* device);

  /**
   * @brief Stop monitoring the currently monitored device.
   */
  void stopMonitoring();

  /**
   * @brief Query whether auto-reconnect monitoring is active.
   *
   * @return @c true if a device is being monitored.
   */
  [[nodiscard]] bool isMonitoring() const;

 signals:
  /**
   * @brief Emitted when the operation succeeds (on any attempt).
   *
   * @param attempt The 1-based attempt number that succeeded.
   */
  void operationSucceeded(int attempt);

  /**
   * @brief Emitted when a single attempt fails but more retries remain.
   *
   * @param attempt     The 1-based attempt number that failed.
   * @param next_delay_ms  Delay in milliseconds before the next retry.
   */
  void retrying(int attempt, int next_delay_ms);

  /**
   * @brief Emitted when all retry attempts have been exhausted.
   *
   * @param total_attempts The total number of attempts made.
   */
  void allRetriesFailed(int total_attempts);

  /**
   * @brief Emitted when an attempt times out.
   *
   * @param attempt    The 1-based attempt number that timed out.
   * @param timeout_ms The timeout value that was exceeded.
   */
  void attemptTimedOut(int attempt, int timeout_ms);

  /**
   * @brief Emitted when auto-reconnect detects a disconnection.
   *
   * @param device_name Name of the device that disconnected.
   */
  void reconnectStarted(const QString& device_name);

  /**
   * @brief Emitted when auto-reconnect succeeds.
   *
   * @param device_name Name of the reconnected device.
   */
  void reconnectSucceeded(const QString& device_name);

  /**
   * @brief Emitted when auto-reconnect fails after all retries.
   *
   * @param device_name Name of the device that could not be reconnected.
   */
  void reconnectFailed(const QString& device_name);

 private slots:
  /**
   * @brief Perform the next retry attempt.
   */
  void performRetry();

  /**
   * @brief Handle timeout expiry for the current attempt.
   */
  void handleTimeout();

  /**
   * @brief Handle state changes on the monitored device.
   *
   * @param new_state The device's new state.
   */
  void onDeviceStateChanged(DeviceInterface::DeviceState new_state);

 private:
  /**
   * @brief Compute the delay for the given attempt number.
   *
   * @param attempt 1-based attempt number.
   * @return Delay in milliseconds.
   */
  [[nodiscard]] int computeDelay(int attempt) const;

  /**
   * @brief Attempt to reconnect the monitored device.
   */
  void attemptReconnect();

  RetryPolicy policy_;               ///< Active retry configuration.
  std::function<bool()> operation_;   ///< Current operation being retried.
  int current_attempt_ = 0;          ///< Current attempt (1-based).
  int timeout_ms_ = 0;               ///< Per-attempt timeout.
  bool retrying_ = false;            ///< True while retry loop is active.

  QTimer retry_timer_;                ///< Timer for inter-retry delays.
  QTimer timeout_timer_;              ///< Timer for per-attempt timeouts.

  DeviceInterface* monitored_device_ = nullptr;  ///< Device being
                                                   ///< monitored.
  bool reconnecting_ = false;         ///< True during auto-reconnect.
};

}  // namespace mwa::hardware

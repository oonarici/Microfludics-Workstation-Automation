/**
 * @file command_queue.h
 * @brief Thread-safe command queue for serial device communication.
 * @author MWA Team
 * @date 2026-03-24
 *
 * Provides a FIFO command queue that executes callable objects one at a time
 * on a dedicated worker thread. Designed for serialising hardware I/O so that
 * only one command is in flight per device at any time.
 *
 * @copyright LGPL-3.0-or-later
 */

#pragma once

#include <QMutex>
#include <QObject>
#include <QQueue>
#include <QThread>
#include <QTimer>

#include <functional>

namespace mwa::hardware {

// Forward declaration for the internal worker.
class CommandQueueWorker;

/**
 * @class CommandQueue
 * @brief FIFO command queue that executes commands on a dedicated worker thread.
 *
 * CommandQueue accepts arbitrary callable objects via enqueue() and runs them
 * one at a time on an internal QThread. This ensures that device I/O is
 * serialised and never blocks the GUI thread.
 *
 * Each command can optionally be given a per-command timeout. If the command
 * does not complete within the timeout period the commandTimedOut() signal is
 * emitted (the command itself continues to run — the caller is merely
 * notified).
 *
 * Call shutdown() to drain the queue and stop the worker thread. After
 * shutdown the queue no longer accepts commands.
 *
 * @note Thread-safe: enqueue(), clear(), and shutdown() may be called from
 *       any thread.
 *
 * @see DeviceInterface
 */
class CommandQueue : public QObject {
  Q_OBJECT

 public:
  /**
   * @brief Construct a CommandQueue and start its worker thread.
   *
   * @param parent Optional QObject parent for Qt ownership management.
   */
  explicit CommandQueue(QObject* parent = nullptr);

  /**
   * @brief Destructor — calls shutdown() if not already shut down.
   */
  ~CommandQueue() override;

  // Non-copyable, non-movable (QObject restriction).
  CommandQueue(const CommandQueue&) = delete;
  CommandQueue& operator=(const CommandQueue&) = delete;
  CommandQueue(CommandQueue&&) = delete;
  CommandQueue& operator=(CommandQueue&&) = delete;

  /**
   * @brief Enqueue a command for execution on the worker thread.
   *
   * The command will be executed in FIFO order. If a @p timeout_ms is
   * provided and the command runs longer than that duration, the
   * commandTimedOut() signal is emitted.
   *
   * @param command  Callable to execute. Must not be null.
   * @param timeout_ms  Per-command timeout in milliseconds. 0 means no
   *                    timeout (the default).
   * @return @c true if the command was enqueued, @c false if the queue
   *         has been shut down or the command is null.
   */
  bool enqueue(std::function<void()> command, int timeout_ms = 0);

  /**
   * @brief Remove all pending (not yet started) commands from the queue.
   *
   * The currently executing command (if any) is not affected.
   */
  void clear();

  /**
   * @brief Drain the queue and stop the worker thread.
   *
   * Pending commands are discarded. If a command is currently executing,
   * shutdown waits for it to complete before returning. After this call
   * enqueue() will return @c false.
   *
   * This method is safe to call multiple times.
   */
  void shutdown();

  /**
   * @brief Query whether the queue has been shut down.
   *
   * @return @c true after shutdown() has been called.
   */
  [[nodiscard]] bool isShutdown() const;

  /**
   * @brief Return the number of pending commands (not including the
   *        currently executing command).
   *
   * @return Pending command count.
   */
  [[nodiscard]] int pendingCount() const;

 signals:
  /**
   * @brief Emitted when a command begins execution.
   */
  void commandStarted();

  /**
   * @brief Emitted when a command completes successfully.
   */
  void commandFinished();

  /**
   * @brief Emitted when a command throws or otherwise fails.
   *
   * @param error A human-readable description of the failure.
   */
  void commandFailed(const QString& error);

  /**
   * @brief Emitted when a command exceeds its per-command timeout.
   *
   * @param timeout_ms The timeout value that was exceeded.
   */
  void commandTimedOut(int timeout_ms);

 private slots:
  /**
   * @brief Start the timeout timer on the caller's thread.
   *
   * @param timeout_ms Timeout duration in milliseconds.
   */
  void startTimeout(int timeout_ms);

  /**
   * @brief Stop the timeout timer on the caller's thread.
   */
  void stopTimeout();

 private:
  /// @cond INTERNAL

  /**
   * @brief Internal entry pairing a callable with its timeout.
   */
  struct Entry {
    std::function<void()> command;  ///< The callable to execute.
    int timeout_ms;                 ///< Per-command timeout (0 = none).
  };

  /**
   * @brief Schedule the worker to process the next command if not already
   *        processing and there is work pending.
   *
   * Must be called with mutex_ held.
   */
  void scheduleProcessing();

  QThread worker_thread_;             ///< Dedicated worker thread.
  CommandQueueWorker* worker_;        ///< Worker object living on worker_thread_.
  mutable QMutex mutex_;              ///< Guards queue_, shutdown_, processing_.
  QQueue<Entry> queue_;               ///< FIFO command queue.
  bool shutdown_;                     ///< True after shutdown() is called.
  bool processing_;                   ///< True while worker is executing.
  QTimer timeout_timer_;              ///< Timer for per-command timeouts.

  friend class CommandQueueWorker;

  /// @endcond
};

}  // namespace mwa::hardware

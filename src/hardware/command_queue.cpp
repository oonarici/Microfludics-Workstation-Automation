/**
 * @file command_queue.cpp
 * @brief Implementation of the thread-safe CommandQueue.
 * @author MWA Team
 * @date 2026-03-24
 *
 * @copyright LGPL-3.0-or-later
 */

#include "hardware/command_queue.h"

#include <QMetaObject>

#include <exception>
#include <utility>

namespace mwa::hardware {

// ---------------------------------------------------------------------------
// CommandQueueWorker — internal QObject that lives on the worker thread.
// ---------------------------------------------------------------------------

/**
 * @class CommandQueueWorker
 * @brief Internal worker object that processes commands on a dedicated thread.
 *
 * Lives on the CommandQueue's worker thread. Receives processNext() calls
 * via queued connections so that it runs within the thread's event loop.
 */
class CommandQueueWorker : public QObject {
  Q_OBJECT

 public:
  /**
   * @brief Construct the worker, storing a back-pointer to the owning queue.
   *
   * @param queue The CommandQueue that owns this worker.
   */
  explicit CommandQueueWorker(CommandQueue* queue)
      : QObject(nullptr), queue_(queue) {}

 signals:
  /**
   * @brief Request the caller-thread timer to start.
   *
   * @param timeout_ms Timeout duration in milliseconds.
   */
  void requestTimerStart(int timeout_ms);

  /**
   * @brief Request the caller-thread timer to stop.
   */
  void requestTimerStop();

 public slots:
  /**
   * @brief Dequeue and execute the next pending command.
   *
   * If more commands remain after execution, schedules itself again via
   * a queued connection to keep the event loop responsive.
   */
  void processNext() {
    CommandQueue::Entry entry;

    {
      QMutexLocker locker(&queue_->mutex_);
      if (queue_->shutdown_ || queue_->queue_.isEmpty()) {
        queue_->processing_ = false;
        return;
      }
      entry = queue_->queue_.dequeue();
    }

    emit queue_->commandStarted();

    // Request the timeout timer start on the caller's thread so it
    // can fire even while the command blocks this worker thread.
    if (entry.timeout_ms > 0) {
      emit requestTimerStart(entry.timeout_ms);
    }

    try {
      entry.command();
      emit queue_->commandFinished();
    } catch (const std::exception& ex) {
      emit queue_->commandFailed(QString::fromStdString(ex.what()));
    } catch (...) {
      emit queue_->commandFailed(QStringLiteral("Unknown error"));
    }

    // Stop the timeout timer.
    if (entry.timeout_ms > 0) {
      emit requestTimerStop();
    }

    // Schedule next command if available.
    {
      QMutexLocker locker(&queue_->mutex_);
      if (!queue_->shutdown_ && !queue_->queue_.isEmpty()) {
        QMetaObject::invokeMethod(this, &CommandQueueWorker::processNext,
                                  Qt::QueuedConnection);
      } else {
        queue_->processing_ = false;
      }
    }
  }

 private:
  CommandQueue* queue_;  ///< Back-pointer to the owning CommandQueue.
};

// ---------------------------------------------------------------------------
// CommandQueue
// ---------------------------------------------------------------------------

CommandQueue::CommandQueue(QObject* parent)
    : QObject(parent),
      worker_(new CommandQueueWorker(this)),
      shutdown_(false),
      processing_(false),
      current_timeout_ms_(0) {
  worker_thread_.setObjectName(QStringLiteral("CommandQueueWorker"));
  worker_->moveToThread(&worker_thread_);

  // The timeout timer lives on this (caller's) thread, so it can fire
  // even while the worker thread is blocked by a long-running command.
  timeout_timer_.setSingleShot(true);
  QObject::connect(&timeout_timer_, &QTimer::timeout, this, [this]() {
    emit commandTimedOut(current_timeout_ms_);
  });

  // Cross-thread connections: worker signals → CommandQueue slots.
  QObject::connect(worker_, &CommandQueueWorker::requestTimerStart,
                   this, &CommandQueue::startTimeout,
                   Qt::QueuedConnection);
  QObject::connect(worker_, &CommandQueueWorker::requestTimerStop,
                   this, &CommandQueue::stopTimeout,
                   Qt::QueuedConnection);

  worker_thread_.start();
}

CommandQueue::~CommandQueue() {
  shutdown();
  delete worker_;
}

bool CommandQueue::enqueue(std::function<void()> command, int timeout_ms) {
  if (!command) {
    return false;
  }

  QMutexLocker locker(&mutex_);
  if (shutdown_) {
    return false;
  }

  queue_.enqueue(Entry{std::move(command), timeout_ms});
  scheduleProcessing();
  return true;
}

void CommandQueue::clear() {
  QMutexLocker locker(&mutex_);
  queue_.clear();
}

void CommandQueue::shutdown() {
  {
    QMutexLocker locker(&mutex_);
    if (shutdown_) {
      return;
    }
    shutdown_ = true;
    queue_.clear();
  }

  worker_thread_.quit();
  worker_thread_.wait();
  timeout_timer_.stop();
}

bool CommandQueue::isShutdown() const {
  QMutexLocker locker(&mutex_);
  return shutdown_;
}

int CommandQueue::pendingCount() const {
  QMutexLocker locker(&mutex_);
  return queue_.size();
}

void CommandQueue::startTimeout(int timeout_ms) {
  current_timeout_ms_ = timeout_ms;
  timeout_timer_.start(timeout_ms);
}

void CommandQueue::stopTimeout() {
  timeout_timer_.stop();
}

void CommandQueue::scheduleProcessing() {
  // Must be called with mutex_ held.
  if (!processing_) {
    processing_ = true;
    QMetaObject::invokeMethod(worker_, &CommandQueueWorker::processNext,
                              Qt::QueuedConnection);
  }
}

}  // namespace mwa::hardware

// Include the moc file for the worker class defined in this .cpp file.
// AUTOMOC generates <filename>.moc for Q_OBJECT classes in .cpp files.
#include "command_queue.moc"

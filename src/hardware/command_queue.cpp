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
      entry = std::move(queue_->queue_.head());
      queue_->queue_.dequeue();
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

    if (entry.timeout_ms > 0) {
      emit requestTimerStop();
    }

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

CommandQueue::CommandQueue(QObject* parent)
    : QObject(parent),
      worker_(new CommandQueueWorker(this)),
      shutdown_(false),
      processing_(false) {
  worker_thread_.setObjectName(QStringLiteral("CommandQueueWorker"));
  worker_->moveToThread(&worker_thread_);

  // The timeout timer lives on this (caller's) thread, so it can fire
  // even while the worker thread is blocked by a long-running command.
  timeout_timer_.setSingleShot(true);
  QObject::connect(&timeout_timer_, &QTimer::timeout, this, [this]() {
    emit commandTimedOut(timeout_timer_.interval());
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
  // Guard against late delivery after shutdown — a queued
  // requestTimerStart may arrive after the worker thread exits.
  if (shutdown_) {
    return;
  }
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

// Required for Q_OBJECT class defined in this .cpp file.
#include "command_queue.moc"

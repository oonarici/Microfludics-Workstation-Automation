/**
 * @file error_handler.cpp
 * @brief Implementation of the ErrorHandler recovery framework.
 * @author MWA Team
 * @date 2026-03-25
 *
 * @copyright LGPL-3.0-or-later
 */

#include "error_handler.h"

#include <algorithm>
#include <cmath>

#include "core/logger.h"

namespace mwa::hardware {

static const QString kLogSource = QStringLiteral("ErrorHandler");

ErrorHandler::ErrorHandler(QObject* parent) : QObject(parent) {
  retry_timer_.setSingleShot(true);
  timeout_timer_.setSingleShot(true);

  connect(&retry_timer_, &QTimer::timeout, this,
          &ErrorHandler::performRetry);
  connect(&timeout_timer_, &QTimer::timeout, this,
          &ErrorHandler::handleTimeout);
}

ErrorHandler::~ErrorHandler() {
  cancel();
  stopMonitoring();
}

void ErrorHandler::setRetryPolicy(const RetryPolicy& policy) {
  policy_ = policy;
}

const RetryPolicy& ErrorHandler::retryPolicy() const {
  return policy_;
}

void ErrorHandler::execute(std::function<bool()> operation,
                           int timeout_ms) {
  cancel();

  operation_ = std::move(operation);
  timeout_ms_ = timeout_ms;
  current_attempt_ = 0;
  retrying_ = true;

  performRetry();
}

void ErrorHandler::cancel() {
  retry_timer_.stop();
  timeout_timer_.stop();
  retrying_ = false;
  current_attempt_ = 0;
  operation_ = nullptr;
}

bool ErrorHandler::isRetrying() const {
  return retrying_;
}

int ErrorHandler::currentAttempt() const {
  return current_attempt_;
}

void ErrorHandler::monitorDevice(DeviceInterface* device) {
  if (device == nullptr) {
    return;
  }

  stopMonitoring();

  monitored_device_ = device;
  connect(monitored_device_, &DeviceInterface::stateChanged, this,
          &ErrorHandler::onDeviceStateChanged);

  mwa::core::Logger::instance().logInfo(
      QStringLiteral("Monitoring device: %1")
          .arg(device->deviceName()),
      kLogSource);
}

void ErrorHandler::stopMonitoring() {
  if (monitored_device_ != nullptr) {
    disconnect(monitored_device_, &DeviceInterface::stateChanged, this,
               &ErrorHandler::onDeviceStateChanged);

    mwa::core::Logger::instance().logInfo(
        QStringLiteral("Stopped monitoring device: %1")
            .arg(monitored_device_->deviceName()),
        kLogSource);

    monitored_device_ = nullptr;
    reconnecting_ = false;
  }
}

bool ErrorHandler::isMonitoring() const {
  return monitored_device_ != nullptr;
}

void ErrorHandler::performRetry() {
  if (!retrying_ || !operation_) {
    return;
  }

  ++current_attempt_;

  if (timeout_ms_ > 0) {
    timeout_timer_.start(timeout_ms_);
  }

  bool success = false;
  try {
    success = operation_();
  } catch (const std::exception& e) {
    mwa::core::Logger::instance().logError(
        QStringLiteral("Attempt %1 threw exception: %2")
            .arg(current_attempt_)
            .arg(QString::fromUtf8(e.what())),
        kLogSource);
    success = false;
  }

  timeout_timer_.stop();

  if (success) {
    mwa::core::Logger::instance().logInfo(
        QStringLiteral("Operation succeeded on attempt %1")
            .arg(current_attempt_),
        kLogSource);
    retrying_ = false;
    int attempt = current_attempt_;
    current_attempt_ = 0;
    operation_ = nullptr;
    emit operationSucceeded(attempt);
    return;
  }

  // Failure path.
  if (current_attempt_ >= policy_.max_attempts) {
    mwa::core::Logger::instance().logError(
        QStringLiteral("All %1 retry attempts exhausted")
            .arg(policy_.max_attempts),
        kLogSource);
    retrying_ = false;
    int attempts = current_attempt_;
    current_attempt_ = 0;
    operation_ = nullptr;
    emit allRetriesFailed(attempts);
    return;
  }

  int delay = computeDelay(current_attempt_);
  mwa::core::Logger::instance().logWarning(
      QStringLiteral("Attempt %1 failed, retrying in %2 ms")
          .arg(current_attempt_)
          .arg(delay),
      kLogSource);
  emit retrying(current_attempt_, delay);
  retry_timer_.start(delay);
}

void ErrorHandler::handleTimeout() {
  if (!retrying_) {
    return;
  }

  mwa::core::Logger::instance().logWarning(
      QStringLiteral("Attempt %1 timed out after %2 ms")
          .arg(current_attempt_)
          .arg(timeout_ms_),
      kLogSource);

  emit attemptTimedOut(current_attempt_, timeout_ms_);

  // Treat timeout as a failure — schedule next retry if attempts remain.
  if (current_attempt_ >= policy_.max_attempts) {
    retrying_ = false;
    int attempts = current_attempt_;
    current_attempt_ = 0;
    operation_ = nullptr;
    emit allRetriesFailed(attempts);
    return;
  }

  int delay = computeDelay(current_attempt_);
  emit retrying(current_attempt_, delay);
  retry_timer_.start(delay);
}

void ErrorHandler::onDeviceStateChanged(
    DeviceInterface::DeviceState new_state) {
  if (monitored_device_ == nullptr) {
    return;
  }

  if (reconnecting_) {
    // We are waiting for a reconnect attempt to complete.
    if (new_state == DeviceInterface::DeviceState::kConnected) {
      reconnecting_ = false;
      mwa::core::Logger::instance().logInfo(
          QStringLiteral("Auto-reconnect succeeded for %1")
              .arg(monitored_device_->deviceName()),
          kLogSource);
      emit reconnectSucceeded(monitored_device_->deviceName());
    }
    return;
  }

  // Not reconnecting — watch for unexpected disconnection or error.
  if (new_state == DeviceInterface::DeviceState::kDisconnected ||
      new_state == DeviceInterface::DeviceState::kError) {
    mwa::core::Logger::instance().logWarning(
        QStringLiteral("Device %1 disconnected unexpectedly, "
                        "starting auto-reconnect")
            .arg(monitored_device_->deviceName()),
        kLogSource);
    emit reconnectStarted(monitored_device_->deviceName());
    attemptReconnect();
  }
}

int ErrorHandler::computeDelay(int attempt) const {
  double delay =
      policy_.base_delay_ms *
      std::pow(policy_.backoff_factor, attempt - 1);
  return std::min(static_cast<int>(delay), policy_.max_delay_ms);
}

void ErrorHandler::attemptReconnect() {
  if (monitored_device_ == nullptr) {
    return;
  }

  reconnecting_ = true;

  auto* device = monitored_device_;
  execute(
      [device]() {
        device->connectDevice();
        // connectDevice is async — return true to indicate the attempt
        // was launched. The actual success is observed via stateChanged.
        return true;
      },
      0);

  // Override: we use a single-attempt execute here. Real success is
  // determined by the stateChanged signal, not the execute() return.
  // If the device doesn't reach kConnected, the monitoring will detect
  // subsequent disconnection/error and retry again.
}

}  // namespace mwa::hardware

/**
 * @file device_manager.cpp
 * @brief Implementation of the DeviceManager singleton.
 * @author MWA Team
 * @date 2026-03-25
 *
 * @copyright LGPL-3.0-or-later
 */

#include "device_manager.h"

#include <QMutexLocker>

namespace mwa::hardware {

DeviceManager::DeviceManager() = default;

DeviceManager& DeviceManager::instance() {
  static DeviceManager manager;
  return manager;
}

bool DeviceManager::registerDevice(DeviceInterface* device) {
  if (device == nullptr) {
    return false;
  }

  QMutexLocker locker(&mutex_);

  auto type = device->deviceType();

  if (devices_.contains(type)) {
    auto* old_device = devices_.take(type);
    disconnectDeviceSignals(old_device);
    old_device->deleteLater();
  }

  device->setParent(this);
  devices_.insert(type, device);
  connectDeviceSignals(device);

  locker.unlock();
  emit deviceRegistered(type);
  return true;
}

bool DeviceManager::removeDevice(DeviceInterface::DeviceType type) {
  QMutexLocker locker(&mutex_);

  if (!devices_.contains(type)) {
    return false;
  }

  auto* device = devices_.take(type);
  disconnectDeviceSignals(device);

  auto dev_state = device->state();
  if (dev_state == DeviceInterface::DeviceState::kConnected ||
      dev_state == DeviceInterface::DeviceState::kConnecting) {
    device->disconnectDevice();
  }

  device->setParent(nullptr);
  device->deleteLater();

  locker.unlock();
  emit deviceRemoved(type);
  return true;
}

DeviceInterface* DeviceManager::device(
    DeviceInterface::DeviceType type) const {
  QMutexLocker locker(&mutex_);
  return devices_.value(type, nullptr);
}

std::vector<DeviceInterface*> DeviceManager::allDevices() const {
  QMutexLocker locker(&mutex_);
  std::vector<DeviceInterface*> result;
  result.reserve(devices_.size());
  for (auto* dev : devices_) {
    result.push_back(dev);
  }
  return result;
}

int DeviceManager::deviceCount() const {
  QMutexLocker locker(&mutex_);
  return devices_.size();
}

void DeviceManager::connectAll() {
  std::vector<DeviceInterface*> to_connect;
  {
    QMutexLocker locker(&mutex_);
    for (auto* dev : devices_) {
      auto s = dev->state();
      if (s != DeviceInterface::DeviceState::kConnected &&
          s != DeviceInterface::DeviceState::kConnecting) {
        to_connect.push_back(dev);
      }
    }
  }
  for (auto* dev : to_connect) {
    dev->connectDevice();
  }
}

void DeviceManager::disconnectAll() {
  std::vector<DeviceInterface*> to_disconnect;
  {
    QMutexLocker locker(&mutex_);
    for (auto* dev : devices_) {
      auto s = dev->state();
      if (s == DeviceInterface::DeviceState::kConnected ||
          s == DeviceInterface::DeviceState::kConnecting) {
        to_disconnect.push_back(dev);
      }
    }
  }
  for (auto* dev : to_disconnect) {
    dev->disconnectDevice();
  }
}

void DeviceManager::connectDeviceSignals(DeviceInterface* device) {
  connect(
      device, &DeviceInterface::stateChanged, this,
      [this, device](DeviceInterface::DeviceState new_state) {
        emit deviceStateChanged(device->deviceType(), new_state);
      });

  connect(
      device, &DeviceInterface::errorOccurred, this,
      [this, device](const QString& message) {
        emit deviceError(device->deviceType(), message);
      });
}

void DeviceManager::disconnectDeviceSignals(DeviceInterface* device) {
  disconnect(device, nullptr, this, nullptr);
}

}  // namespace mwa::hardware

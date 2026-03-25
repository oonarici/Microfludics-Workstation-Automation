/**
 * @file device_manager.h
 * @brief Singleton manager that owns and coordinates all hardware devices.
 * @author MWA Team
 * @date 2026-03-25
 *
 * Provides centralised registration, lookup, and lifecycle management
 * for every DeviceInterface instance in the application.
 *
 * @copyright LGPL-3.0-or-later
 */

#pragma once

#include <QHash>
#include <QMutex>
#include <QObject>

#include <vector>

#include "hardware/device_interface.h"

namespace mwa::hardware {

/**
 * @class DeviceManager
 * @brief Singleton that owns and coordinates all hardware device controllers.
 *
 * DeviceManager maintains a registry of DeviceInterface pointers keyed by
 * DeviceType. It provides batch operations (connectAll / disconnectAll)
 * and forwards per-device state-change and error signals so that GUI
 * components can observe the entire device fleet through a single object.
 *
 * Devices are registered via registerDevice() and looked up via device()
 * or allDevices(). The manager takes Qt parent-ownership of registered
 * devices, so they are destroyed when the manager is destroyed.
 *
 * @note Thread-safe: all public methods guard shared state with a mutex.
 *
 * @see DeviceInterface
 * @see CommandQueue
 */
class DeviceManager : public QObject {
  Q_OBJECT

 public:
  /**
   * @brief Access the singleton DeviceManager instance.
   *
   * The instance is created on first call and lives until application
   * exit.
   *
   * @return Reference to the singleton DeviceManager.
   */
  static DeviceManager& instance();

  /**
   * @brief Register a device controller with the manager.
   *
   * The manager takes Qt parent-ownership of the device. If a device
   * of the same type is already registered it is removed first (the old
   * device is scheduled for deletion via deleteLater()).
   *
   * @param device Non-null pointer to the device to register.
   * @return @c true if the device was registered, @c false if @p device
   *         is null.
   */
  bool registerDevice(DeviceInterface* device);

  /**
   * @brief Remove a previously registered device.
   *
   * The device is disconnected (if connected), removed from the
   * registry, and scheduled for deletion via deleteLater().
   *
   * @param type The DeviceType to remove.
   * @return @c true if a device of that type was found and removed.
   */
  bool removeDevice(DeviceInterface::DeviceType type);

  /**
   * @brief Look up the device registered for a given type.
   *
   * @param type The DeviceType to look up.
   * @return Pointer to the device, or @c nullptr if none is registered.
   */
  [[nodiscard]] DeviceInterface* device(
      DeviceInterface::DeviceType type) const;

  /**
   * @brief Return all currently registered devices.
   *
   * @return A vector of non-owning pointers to every registered device.
   */
  [[nodiscard]] std::vector<DeviceInterface*> allDevices() const;

  /**
   * @brief Return the number of currently registered devices.
   *
   * @return Device count.
   */
  [[nodiscard]] int deviceCount() const;

  /**
   * @brief Initiate connection on every registered device.
   *
   * Calls connectDevice() on each registered device that is not already
   * connected or connecting.
   */
  void connectAll();

  /**
   * @brief Initiate disconnection on every registered device.
   *
   * Calls disconnectDevice() on each registered device that is
   * currently connected or connecting.
   */
  void disconnectAll();

  // Non-copyable, non-movable singleton.
  DeviceManager(const DeviceManager&) = delete;
  DeviceManager& operator=(const DeviceManager&) = delete;
  DeviceManager(DeviceManager&&) = delete;
  DeviceManager& operator=(DeviceManager&&) = delete;

 signals:
  /**
   * @brief Emitted after a device is registered with the manager.
   *
   * @param type The DeviceType of the newly registered device.
   */
  void deviceRegistered(
      mwa::hardware::DeviceInterface::DeviceType type);

  /**
   * @brief Emitted after a device is removed from the manager.
   *
   * @param type The DeviceType of the removed device.
   */
  void deviceRemoved(
      mwa::hardware::DeviceInterface::DeviceType type);

  /**
   * @brief Emitted when any registered device changes connection state.
   *
   * @param type      The DeviceType whose state changed.
   * @param new_state The new DeviceState value.
   */
  void deviceStateChanged(
      mwa::hardware::DeviceInterface::DeviceType type,
      mwa::hardware::DeviceInterface::DeviceState new_state);

  /**
   * @brief Emitted when any registered device reports an error.
   *
   * @param type    The DeviceType that reported the error.
   * @param message A human-readable error description.
   */
  void deviceError(
      mwa::hardware::DeviceInterface::DeviceType type,
      const QString& message);

 private:
  /**
   * @brief Private constructor — use instance() instead.
   */
  DeviceManager();

  /**
   * @brief Connect internal signal forwarding for a device.
   *
   * @param device The device whose signals should be forwarded.
   */
  void connectDeviceSignals(DeviceInterface* device);

  /**
   * @brief Disconnect internal signal forwarding for a device.
   *
   * @param device The device whose signals should be disconnected.
   */
  void disconnectDeviceSignals(DeviceInterface* device);

  mutable QMutex mutex_;  ///< Guards the device registry.

  /// Device registry keyed by DeviceType.
  QHash<DeviceInterface::DeviceType, DeviceInterface*> devices_;
};

}  // namespace mwa::hardware

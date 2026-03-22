/**
 * @file settings_manager.h
 * @brief Singleton settings manager wrapping QSettings with INI format.
 * @author MWA Team
 * @date 2026-03-22
 *
 * Provides a centralized, group-based settings facility that persists
 * configuration using QSettings in INI format for cross-platform
 * consistency.
 *
 * @copyright LGPL-3.0-or-later
 */

#pragma once

#include <QObject>
#include <QSettings>
#include <QString>
#include <QVariant>

namespace mwa::core {

/**
 * @class SettingsManager
 * @brief Singleton that manages application settings grouped by device.
 *
 * SettingsManager wraps QSettings with INI format and organizes
 * settings into groups (e.g. "LED", "Pump", "Camera"). It emits a
 * signal whenever a setting changes, allowing GUI components to react.
 *
 * @note Settings are written through immediately via QSettings::sync()
 *       when saveAll() is called.
 *
 * @see QSettings
 */
class SettingsManager : public QObject {
  Q_OBJECT

 public:
  /**
   * @brief Access the singleton SettingsManager instance.
   *
   * @return Reference to the singleton SettingsManager.
   */
  static SettingsManager& instance();

  /**
   * @brief Set a value within a device settings group.
   *
   * If the new value differs from the existing value, the
   * settingChanged() signal is emitted.
   *
   * @param group The settings group name (e.g. "LED", "Pump").
   * @param key   The setting key within the group.
   * @param value The value to store.
   */
  void setValue(const QString& group, const QString& key,
                const QVariant& value);

  /**
   * @brief Retrieve a value from a device settings group.
   *
   * @param group        The settings group name.
   * @param key          The setting key within the group.
   * @param default_value Value returned if the key does not exist.
   * @return The stored value, or @p default_value if not found.
   */
  QVariant value(const QString& group, const QString& key,
                 const QVariant& default_value = QVariant()) const;

  /**
   * @brief Flush all pending settings to persistent storage.
   */
  void saveAll();

  /**
   * @brief Reload all settings from persistent storage.
   */
  void loadAll();

  // Non-copyable, non-movable singleton.
  SettingsManager(const SettingsManager&) = delete;
  SettingsManager& operator=(const SettingsManager&) = delete;
  SettingsManager(SettingsManager&&) = delete;
  SettingsManager& operator=(SettingsManager&&) = delete;

 signals:
  /**
   * @brief Emitted when a setting value changes.
   *
   * @param group The settings group that was modified.
   * @param key   The key that was modified.
   * @param value The new value.
   */
  void settingChanged(const QString& group, const QString& key,
                      const QVariant& value);

 private:
  /**
   * @brief Private constructor — use instance() instead.
   */
  SettingsManager();

  QSettings settings_;  ///< Underlying INI-format QSettings store.
};

}  // namespace mwa::core

/**
 * @file settings_manager.cpp
 * @brief Implementation of the singleton SettingsManager class.
 * @author MWA Team
 * @date 2026-03-22
 *
 * @copyright LGPL-3.0-or-later
 */

#include "settings_manager.h"

namespace mwa::core {

SettingsManager::SettingsManager()
    : settings_(QSettings::IniFormat, QSettings::UserScope,
                QStringLiteral("MWA"),
                QStringLiteral("MicrofluidicsWorkstation")) {}

SettingsManager& SettingsManager::instance() {
  static SettingsManager manager;
  return manager;
}

void SettingsManager::setValue(const QString& group,
                               const QString& key,
                               const QVariant& value) {
  QString full_key = group + "/" + key;
  QVariant old_value = settings_.value(full_key);
  settings_.setValue(full_key, value);
  if (old_value != value) {
    emit settingChanged(group, key, value);
  }
}

QVariant SettingsManager::value(const QString& group,
                                const QString& key,
                                const QVariant& default_value) const {
  QString full_key = group + "/" + key;
  return settings_.value(full_key, default_value);
}

void SettingsManager::saveAll() {
  settings_.sync();
}

void SettingsManager::loadAll() {
  settings_.sync();
}

}  // namespace mwa::core

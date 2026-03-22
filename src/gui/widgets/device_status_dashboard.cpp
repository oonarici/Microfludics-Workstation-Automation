/**
 * @file device_status_dashboard.cpp
 * @brief Implementation of the DeviceStatusDashboard class.
 * @author MWA Team
 * @date 2026-03-22
 *
 * @copyright LGPL-3.0-or-later
 */

#include "gui/widgets/device_status_dashboard.h"

#include <QHBoxLayout>
#include <QVBoxLayout>

namespace mwa::gui {

using DeviceType =
    mwa::hardware::DeviceInterface::DeviceType;
using DeviceState =
    mwa::hardware::DeviceInterface::DeviceState;

DeviceStatusDashboard::DeviceStatusDashboard(QWidget* parent)
    : QWidget(parent) {
  setObjectName(QStringLiteral("deviceStatusDashboard"));
  setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);

  auto* root_layout = new QVBoxLayout(this);
  root_layout->setContentsMargins(0, 0, 0, 0);
  root_layout->setSpacing(0);

  auto* group = new QGroupBox(
      QStringLiteral("Device Status"), this);
  group->setObjectName(QStringLiteral("grpDeviceStatus"));

  auto* group_layout = new QVBoxLayout(group);
  group_layout->setContentsMargins(8, 8, 8, 8);
  group_layout->setSpacing(4);

  // Create cards for all 6 device types.
  struct CardDef {
    DeviceType type;
    QString name;
    QString icon;
  };
  const CardDef card_defs[] = {
      {DeviceType::kLed,
       QStringLiteral("LED Light Source"),
       QStringLiteral("\U0001F4A1")},
      {DeviceType::kPump,
       QStringLiteral("Syringe Pump"),
       QStringLiteral("\U0001F489")},
      {DeviceType::kSignalGenerator,
       QStringLiteral("Signal Generator"),
       QStringLiteral("\U0001F30A")},
      {DeviceType::kNetworkAnalyzer,
       QStringLiteral("Network Analyzer"),
       QStringLiteral("\U0001F4E1")},
      {DeviceType::kCamera,
       QStringLiteral("Camera"),
       QStringLiteral("\U0001F4F7")},
      {DeviceType::kStage,
       QStringLiteral("XYZ Stage"),
       QStringLiteral("\u2630")},
  };

  for (const auto& def : card_defs) {
    auto card = createCard(def.type, def.name, def.icon, group);
    group_layout->addWidget(card.frame);
    cards_.insert(def.type, card);
  }

  group_layout->addStretch();
  root_layout->addWidget(group);
  root_layout->addStretch();
}

DeviceStatusDashboard::DeviceCard
DeviceStatusDashboard::createCard(
    DeviceType type,
    const QString& display_name,
    const QString& icon_text,
    QWidget* parent) {
  DeviceCard card;

  // Suffix for objectName based on device type.
  QString suffix;
  switch (type) {
    case DeviceType::kLed:
      suffix = QStringLiteral("Led");
      break;
    case DeviceType::kPump:
      suffix = QStringLiteral("Pump");
      break;
    case DeviceType::kSignalGenerator:
      suffix = QStringLiteral("SignalGenerator");
      break;
    case DeviceType::kNetworkAnalyzer:
      suffix = QStringLiteral("NetworkAnalyzer");
      break;
    case DeviceType::kCamera:
      suffix = QStringLiteral("Camera");
      break;
    case DeviceType::kStage:
      suffix = QStringLiteral("Stage");
      break;
  }

  card.frame = new QFrame(parent);
  card.frame->setObjectName(
      QStringLiteral("cardFrame") + suffix);
  card.frame->setFixedHeight(48);
  card.frame->setFrameShape(QFrame::StyledPanel);

  auto* layout = new QHBoxLayout(card.frame);
  layout->setContentsMargins(4, 4, 8, 4);
  layout->setSpacing(8);

  card.icon = new QLabel(icon_text, card.frame);
  card.icon->setObjectName(
      QStringLiteral("lblIcon") + suffix);
  card.icon->setFixedSize(20, 20);
  card.icon->setAlignment(Qt::AlignCenter);
  card.icon->setStyleSheet(
      QStringLiteral("font-size: 14pt;"));
  layout->addWidget(card.icon);

  card.name = new QLabel(display_name, card.frame);
  card.name->setObjectName(
      QStringLiteral("lblName") + suffix);
  card.name->setSizePolicy(QSizePolicy::Expanding,
                           QSizePolicy::Preferred);
  card.name->setAlignment(
      Qt::AlignVCenter | Qt::AlignLeft);
  card.name->setStyleSheet(
      QStringLiteral("font-size: 12pt;"));
  layout->addWidget(card.name);

  card.dot = new QLabel(card.frame);
  card.dot->setObjectName(
      QStringLiteral("lblDot") + suffix);
  card.dot->setFixedSize(12, 12);
  card.dot->setStyleSheet(QStringLiteral(
      "background-color: #95A5A6; border-radius: 6px;"));
  layout->addWidget(card.dot);

  card.state_text = new QLabel(
      QStringLiteral("Disconnected"), card.frame);
  card.state_text->setObjectName(
      QStringLiteral("lblState") + suffix);
  card.state_text->setFixedWidth(90);
  card.state_text->setAlignment(
      Qt::AlignVCenter | Qt::AlignRight);
  card.state_text->setStyleSheet(
      QStringLiteral("font-size: 10pt; color: #95A5A6;"));
  layout->addWidget(card.state_text);

  return card;
}

void DeviceStatusDashboard::registerDevice(
    mwa::hardware::DeviceInterface* device) {
  if (device == nullptr) {
    return;
  }
  DeviceType type = device->deviceType();

  // Disconnect old device if one was registered.
  if (cards_.contains(type) &&
      cards_[type].device != nullptr) {
    disconnect(cards_[type].device, nullptr, this, nullptr);
  }

  if (!cards_.contains(type)) {
    return;
  }

  cards_[type].device = device;

  connect(device,
          &mwa::hardware::DeviceInterface::stateChanged,
          this, [this, type](DeviceState state) {
            updateCardState(type, state);
          });

  updateCardState(type, device->state());
}

void DeviceStatusDashboard::unregisterDevice(
    DeviceType type) {
  if (!cards_.contains(type)) {
    return;
  }

  auto& card = cards_[type];
  if (card.device != nullptr) {
    disconnect(card.device, nullptr, this, nullptr);
    card.device = nullptr;
  }

  updateCardState(type, DeviceState::kDisconnected);
}

void DeviceStatusDashboard::updateCardState(
    DeviceType type, DeviceState state) {
  if (!cards_.contains(type)) {
    return;
  }

  auto& card = cards_[type];
  QString color;
  QString text;

  switch (state) {
    case DeviceState::kDisconnected:
      color = QStringLiteral("#95A5A6");
      text = QStringLiteral("Disconnected");
      break;
    case DeviceState::kConnecting:
      color = QStringLiteral("#F39C12");
      text = QStringLiteral("Connecting...");
      break;
    case DeviceState::kConnected:
      color = QStringLiteral("#27AE60");
      text = QStringLiteral("Connected");
      break;
    case DeviceState::kError:
      color = QStringLiteral("#E74C3C");
      text = QStringLiteral("Error");
      break;
  }

  card.dot->setStyleSheet(
      QStringLiteral("background-color: %1; "
                     "border-radius: 6px;")
          .arg(color));
  card.state_text->setText(text);
  card.state_text->setStyleSheet(
      QStringLiteral("font-size: 10pt; color: %1;")
          .arg(color));
}

}  // namespace mwa::gui

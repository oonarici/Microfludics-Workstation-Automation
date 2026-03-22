/**
 * @file device_status_dashboard.h
 * @brief Dashboard widget showing connection status of all devices.
 * @author MWA Team
 * @date 2026-03-22
 *
 * Declares the DeviceStatusDashboard widget that replaces the left
 * dock placeholder in MainWindow. It displays a compact card for
 * each of the six device types with a color-coded state indicator.
 *
 * @copyright LGPL-3.0-or-later
 */

#pragma once

#include <QFrame>
#include <QGroupBox>
#include <QLabel>
#include <QMap>
#include <QWidget>

#include "hardware/device_interface.h"

namespace mwa::gui {

/**
 * @class DeviceStatusDashboard
 * @brief Compact overview panel showing all device connection states.
 *
 * Displays six device cards (one per DeviceType) arranged vertically
 * inside a QGroupBox. Each card shows the device name and a
 * color-coded state indicator (gray=disconnected, yellow=connecting,
 * green=connected, red=error).
 *
 * @see mwa::hardware::DeviceInterface
 */
class DeviceStatusDashboard : public QWidget {
  Q_OBJECT

 public:
  /**
   * @brief Construct the device status dashboard.
   *
   * Creates all six device cards in their initial disconnected state.
   *
   * @param parent Optional parent widget.
   */
  explicit DeviceStatusDashboard(QWidget* parent = nullptr);

 public slots:
  /**
   * @brief Register a device and connect to its state signals.
   *
   * Enables the card for the device's type and updates it whenever
   * the device emits stateChanged. If a device of the same type was
   * already registered, the old connection is replaced.
   *
   * @param device Pointer to the device interface (not owned).
   */
  void registerDevice(
      mwa::hardware::DeviceInterface* device);

  /**
   * @brief Unregister a device and reset its card.
   *
   * Disconnects state signals and resets the card to the
   * disconnected visual state.
   *
   * @param type The device type to unregister.
   */
  void unregisterDevice(
      mwa::hardware::DeviceInterface::DeviceType type);

 signals:
  /**
   * @brief Emitted when a device card is clicked.
   *
   * Reserved for future use by MainWindow to expand device
   * control panels.
   *
   * @param type The device type whose card was clicked.
   */
  void cardClicked(
      mwa::hardware::DeviceInterface::DeviceType type);

 private:
  /**
   * @brief Internal representation of a single device card.
   */
  struct DeviceCard {
    QFrame* frame{nullptr};     ///< Card container frame.
    QLabel* icon{nullptr};      ///< Device type icon label.
    QLabel* name{nullptr};      ///< Device name label.
    QLabel* dot{nullptr};       ///< Color-coded state dot.
    QLabel* state_text{nullptr};  ///< State text label.
    mwa::hardware::DeviceInterface* device{nullptr};  ///< Registered device.
  };

  /**
   * @brief Create a single device card widget.
   *
   * @param type         The device type for this card.
   * @param display_name Human-readable device name.
   * @param icon_text    Unicode icon or fallback text.
   * @param parent       Parent widget.
   * @return The constructed DeviceCard.
   */
  DeviceCard createCard(
      mwa::hardware::DeviceInterface::DeviceType type,
      const QString& display_name,
      const QString& icon_text,
      QWidget* parent);

  /**
   * @brief Update a card's visual state.
   *
   * @param type  The device type to update.
   * @param state The new device state.
   */
  void updateCardState(
      mwa::hardware::DeviceInterface::DeviceType type,
      mwa::hardware::DeviceInterface::DeviceState state);

  /// Map from device type to its card widgets.
  QMap<mwa::hardware::DeviceInterface::DeviceType, DeviceCard>
      cards_;
};

}  // namespace mwa::gui

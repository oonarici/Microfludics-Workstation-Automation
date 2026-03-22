# Device Status Dashboard Design Specification

**Task:** MWA-02-F
**Requirement:** GUI-REQ-007
**Designer:** UX Agent
**Date:** 2026-03-22
**Status:** APPROVED by Lead

---

## Overview

The Device Status Dashboard (`DeviceStatusDashboard`) is a compact `QWidget` inside the existing `dockDevicePanels` `QDockWidget`, replacing its placeholder. It presents a vertical list of six device cards — one per `DeviceType` — each showing the device name and connection state via a color-coded indicator. It is a read-only overview panel; device controls will be added in Sprint 3.

---

## 1. ASCII Layout

```
┌─ Device Panels ──────────────────────────── [float][X] ─┐
│                                                          │
│ ┌─ Device Status ──────────────────────────────────────┐ │
│ │                                                      │ │
│ │  ┌──────────────────────────────────────────────┐    │ │
│ │  │ [●]  LED Light Source          ● Disconnected│    │ │
│ │  └──────────────────────────────────────────────┘    │ │
│ │  ┌──────────────────────────────────────────────┐    │ │
│ │  │ [●]  Syringe Pump              ● Disconnected│    │ │
│ │  └──────────────────────────────────────────────┘    │ │
│ │  ┌──────────────────────────────────────────────┐    │ │
│ │  │ [●]  Signal Generator          ● Disconnected│    │ │
│ │  └──────────────────────────────────────────────┘    │ │
│ │  ┌──────────────────────────────────────────────┐    │ │
│ │  │ [●]  Network Analyzer          ● Disconnected│    │ │
│ │  └──────────────────────────────────────────────┘    │ │
│ │  ┌──────────────────────────────────────────────┐    │ │
│ │  │ [●]  Camera                    ● Disconnected│    │ │
│ │  └──────────────────────────────────────────────┘    │ │
│ │  ┌──────────────────────────────────────────────┐    │ │
│ │  │ [●]  XYZ Stage                 ● Disconnected│    │ │
│ │  └──────────────────────────────────────────────┘    │ │
│ │                                                      │ │
│ └──────────────────────────────────────────────────────┘ │
│                                                          │
│  (future device control panels go below)                 │
└──────────────────────────────────────────────────────────┘
```

### Card Row Detail (48px tall)

```
┌──────────────────────────────────────────────────┐
│  [icon 20×20]  Device Name         [dot 12×12] State │
└──────────────────────────────────────────────────┘
```

---

## 2. Widget Table

### DeviceStatusDashboard (root)

| Widget | Type | objectName | Properties | Signals |
|--------|------|------------|------------|---------|
| Dashboard root | `QWidget` | `deviceStatusDashboard` | Layout: `QVBoxLayout`, margin: 0, spacing: 0 | — |
| Device Status group | `QGroupBox` | `grpDeviceStatus` | Title: "Device Status", layout: `QVBoxLayout`, 8px margin, 4px spacing | — |

### Device Cards (6 cards, pattern: substitute {Device})

| Widget | Type | objectName | Properties | Signals |
|--------|------|------------|------------|---------|
| Card frame | `QFrame` | `cardFrame{Device}` | Fixed height: 48px, layout: `QHBoxLayout`, margins: 4/8/4/8, spacing: 8px | — |
| Icon label | `QLabel` | `lblIcon{Device}` | Fixed 20×20px, alignment: center, font: 14pt | — |
| Name label | `QLabel` | `lblName{Device}` | Font: 12pt, sizePolicy: Expanding, alignment: AlignVCenter|AlignLeft | — |
| State dot | `QLabel` | `lblDot{Device}` | Fixed 12×12px, border-radius: 6px, background: `#95A5A6` | — |
| State text | `QLabel` | `lblState{Device}` | Font: 10pt, fixed width: 90px, alignment: AlignVCenter|AlignRight | — |

### Device Names and Icons

| DeviceType | Display Name | Icon (Unicode) |
|---|---|---|
| `kLed` | LED Light Source | 💡 |
| `kPump` | Syringe Pump | 💉 |
| `kSignalGenerator` | Signal Generator | 🌊 |
| `kNetworkAnalyzer` | Network Analyzer | 📡 |
| `kCamera` | Camera | 📷 |
| `kStage` | XYZ Stage | ☰ |

### State Colors

| DeviceState | Dot Color | State Text | Text Color |
|---|---|---|---|
| `kDisconnected` | `#95A5A6` | "Disconnected" | `#95A5A6` |
| `kConnecting` | `#F39C12` | "Connecting..." | `#F39C12` |
| `kConnected` | `#27AE60` | "Connected" | `#27AE60` |
| `kError` | `#E74C3C` | "Error" | `#E74C3C` |

---

## 3. State Table

| UI State | Trigger | Visual Change |
|----------|---------|---------------|
| **Initial** | Constructed | All 6 cards show "Disconnected" in gray |
| **Device Connecting** | `stateChanged(kConnecting)` | Dot yellow, text "Connecting..." |
| **Device Connected** | `stateChanged(kConnected)` | Dot green, text "Connected" |
| **Device Error** | `stateChanged(kError)` | Dot red, text "Error" |
| **Device Disconnected** | `stateChanged(kDisconnected)` | Dot gray, text "Disconnected" |

---

## 4. Interaction List

| User Action | UI Response |
|---|---|
| Widget constructed | All 6 cards in disconnected state |
| `registerDevice(DeviceInterface*)` called | Card for that type connects to stateChanged |
| `stateChanged` emitted | Matching card dot/text updated |

---

## 5. Keyboard Shortcuts

No application-level shortcuts. Standard Tab navigation between cards.

---

## 6. Public API

```cpp
class DeviceStatusDashboard : public QWidget {
  Q_OBJECT
public:
  explicit DeviceStatusDashboard(QWidget* parent = nullptr);

public slots:
  void registerDevice(mwa::hardware::DeviceInterface* device);
  void unregisterDevice(mwa::hardware::DeviceInterface::DeviceType type);

signals:
  void cardClicked(mwa::hardware::DeviceInterface::DeviceType type);
};
```

---

## 7. MainWindow Integration

Replace placeholder in `MainWindow::createDocks()`:
```
dock_device_panels_->setWidget(new DeviceStatusDashboard(this));
```
Remove `wgtDevicePanelsPlaceholder` and `lblDevicePanelsPlaceholder`.

# Cetoni Nemesys SDK Specification -- MWA Project

> **Version:** 1.0
> **Date:** 2026-04-02
> **Purpose:** Reference for implementing the `PumpController` driver and enhancing
> `MockPumpController` in the MWA application.
> **SDK:** Cetoni QmixSDK (C API)

---

## Table of Contents

1. [Overview](#1-overview)
2. [SDK Architecture](#2-sdk-architecture)
3. [C API Reference](#3-c-api-reference)
4. [Error Model](#4-error-model)
5. [Units & Conversions](#5-units--conversions)
6. [Syringe Specifications](#6-syringe-specifications)
7. [Typical Workflows](#7-typical-workflows)
8. [Platform Availability](#8-platform-availability)
9. [CMake Integration](#9-cmake-integration)
10. [Mock Data Patterns](#10-mock-data-patterns)
11. [MWA Interface Mapping](#11-mwa-interface-mapping)
12. [References](#12-references)

---

## 1. Overview

The **Cetoni Nemesys** is a modular, high-precision syringe pump system designed for
microfluidics and laboratory automation. Unlike SCPI-based instruments, it uses a
proprietary compiled **C API** called **QmixSDK** to communicate with pump modules
over a CAN bus (typically USB-to-CAN adapter).

### System Architecture

```
PC (USB) ──── Cetoni Base Module (USB-to-CAN) ──── CAN Bus
                                                      │
                                              ┌───────┴───────┐
                                              │ Pump Module 1 │
                                              │ (Nemesys S)   │
                                              └───────────────┘
                                              ┌───────────────┐
                                              │ Pump Module 2 │
                                              │ (Nemesys M)   │
                                              └───────────────┘
                                              ... (up to 16 modules)
```

### Nemesys Pump Series

| Model | Syringe Volume Range | Min Flow Rate | Max Flow Rate | Resolution |
|-------|---------------------|---------------|---------------|------------|
| Nemesys S | 1 µL -- 1 mL | 0.4 nL/s | 12.5 µL/s | < 1 nL |
| Nemesys M | 10 µL -- 10 mL | 3.3 nL/s | 125 µL/s | < 10 nL |
| Nemesys XL | 100 µL -- 50 mL | 33 nL/s | 416 µL/s | < 100 nL |

### Key Characteristics

- **Communication**: Not serial/SCPI -- uses compiled C/C++ SDK with CAN bus backend
- **Positioning**: Stepper-motor-driven syringe plunger with sub-microliter precision
- **Modes**: Volume dosing (dispense/aspirate fixed volume), continuous flow generation
- **Valves**: Optional rotary valve for switching between inlet/outlet ports

---

## 2. SDK Architecture

The QmixSDK is organized into functional modules with C-style function prefixes:

| Module | Prefix | Purpose |
|--------|--------|---------|
| Bus/System | `LCB_*` | Device bus management (open, start, stop, close) |
| Pump Control | `LCP_*` | Syringe pump operations (dose, aspirate, flow) |
| Valve Control | `LCV_*` | Rotary valve switching |
| Digital I/O | `LCDI_*` / `LCDO_*` | Digital input/output channels |
| Analog I/O | `LCAI_*` / `LCAO_*` | Analog input/output channels |
| Controller | `LCC_*` | PID controller channels |
| Error | `LCB_*` | Error codes and descriptions |

### Handle System

SDK functions operate on opaque handles:

```c
typedef long long dev_hdl;      // Device handle (pump, valve, I/O)
typedef long long lca_hdl;      // Analog channel handle
typedef long long lcc_hdl;      // Controller handle
```

Handles are obtained from lookup functions after bus initialization.

### Thread Safety

- SDK functions are **NOT thread-safe** for the same device
- Different devices on the same bus CAN be accessed from different threads
- **MWA strategy**: Use `CommandQueue` to serialize all calls for a given pump

---

## 3. C API Reference

### 3.1 Bus Management (LCB_*)

These functions manage the device bus (CAN network) lifecycle.

```c
/**
 * @brief Open the device bus configuration.
 * @param config_path  Path to device configuration folder
 *                     (created by Cetoni Elements software)
 * @return Error code (ERR_NOERR = 0 on success)
 */
long LCB_Open(const char* config_path);

/**
 * @brief Start bus communication. Must be called after LCB_Open().
 * @return Error code
 */
long LCB_Start();

/**
 * @brief Stop bus communication.
 * @return Error code
 */
long LCB_Stop();

/**
 * @brief Close the device bus and free resources.
 * @return Error code
 */
long LCB_Close();

/**
 * @brief Check if bus communication is running.
 * @return 1 if started, 0 if stopped
 */
int LCB_IsStarted();

/**
 * @brief Get the number of pump devices on the bus.
 * @return Number of pump modules found
 */
long LCB_GetPumpCount();

/**
 * @brief Get handle to a specific pump device by index.
 * @param index  Zero-based pump index
 * @param handle Output: device handle
 * @return Error code
 */
long LCB_GetPumpHandle(int index, dev_hdl* handle);

/**
 * @brief Look up a pump device by its name string.
 * @param name   Device name from configuration (e.g., "neMESYS1_Pump")
 * @param handle Output: device handle
 * @return Error code
 */
long LCB_LookupPumpByName(const char* name, dev_hdl* handle);
```

### 3.2 Pump Configuration (LCP_Set*)

Configure the syringe and unit system before first use.

```c
/**
 * @brief Set syringe parameters for a pump.
 * @param handle         Pump device handle
 * @param inner_diameter Inner diameter of the syringe in millimetres
 * @param stroke         Maximum plunger travel in millimetres
 * @return Error code
 */
long LCP_SetSyringeParam(dev_hdl handle,
                          double inner_diameter,
                          double stroke);

/**
 * @brief Get the currently configured syringe inner diameter.
 * @param handle Pump device handle
 * @return Inner diameter in millimetres
 */
double LCP_GetSyringeInnerDiameter(dev_hdl handle);

/**
 * @brief Get the maximum syringe stroke.
 * @param handle Pump device handle
 * @return Stroke in millimetres
 */
double LCP_GetMaxSyringeStroke(dev_hdl handle);

/**
 * @brief Set the volume unit for subsequent volume-related calls.
 * @param handle Pump device handle
 * @param prefix SI prefix enum (MILLI = -3, MICRO = -6, NANO = -9)
 * @param unit   Volume unit enum (UNIT_LITRE = 68)
 * @return Error code
 *
 * Example: LCP_SetVolumeUnit(pump, MICRO, UNIT_LITRE) → µL
 */
long LCP_SetVolumeUnit(dev_hdl handle, int prefix, int unit);

/**
 * @brief Set the flow rate unit for subsequent flow-related calls.
 * @param handle     Pump device handle
 * @param prefix     SI prefix for volume (MICRO = -6)
 * @param volume_unit Volume unit (UNIT_LITRE = 68)
 * @param time_unit  Time unit (PER_SECOND = 1, PER_MINUTE = 60)
 * @return Error code
 *
 * Example: LCP_SetFlowUnit(pump, MICRO, UNIT_LITRE, PER_MINUTE) → µL/min
 */
long LCP_SetFlowUnit(dev_hdl handle, int prefix,
                      int volume_unit, int time_unit);
```

### 3.3 Pump Operations (LCP_Aspirate/Dispense/...)

```c
/**
 * @brief Dispense (push) a specific volume at a given flow rate.
 *
 * The pump moves the plunger forward, pushing fluid out of the syringe.
 * Volume and flow rate use the currently configured units.
 *
 * @param handle    Pump device handle
 * @param volume    Volume to dispense (in configured volume unit)
 * @param flow_rate Flow rate (in configured flow unit)
 * @return Error code
 */
long LCP_Dispense(dev_hdl handle, double volume, double flow_rate);

/**
 * @brief Aspirate (pull) a specific volume at a given flow rate.
 *
 * The pump retracts the plunger, drawing fluid into the syringe.
 *
 * @param handle    Pump device handle
 * @param volume    Volume to aspirate
 * @param flow_rate Flow rate
 * @return Error code
 */
long LCP_Aspirate(dev_hdl handle, double volume, double flow_rate);

/**
 * @brief Generate continuous flow at a given rate.
 *
 * Unlike Dispense/Aspirate, this does not specify a target volume.
 * The pump runs until stopped or the syringe is empty/full.
 * Positive flow_rate → dispense direction.
 * Negative flow_rate → aspirate direction.
 *
 * @param handle    Pump device handle
 * @param flow_rate Flow rate (sign determines direction)
 * @return Error code
 */
long LCP_GenerateFlow(dev_hdl handle, double flow_rate);

/**
 * @brief Immediately stop the pump.
 * @param handle Pump device handle
 * @return Error code
 */
long LCP_StopPumping(dev_hdl handle);

/**
 * @brief Move the syringe to a specific fill level.
 * @param handle    Pump device handle
 * @param level     Target fill level (in configured volume unit)
 * @param flow_rate Flow rate for the move
 * @return Error code
 */
long LCP_SetFillLevel(dev_hdl handle, double level, double flow_rate);
```

### 3.4 Status Queries (LCP_Get*/LCP_Is*)

```c
/**
 * @brief Get the current syringe fill level.
 * @param handle Pump device handle
 * @return Fill level in configured volume unit (0 = empty, max = full)
 */
double LCP_GetFillLevel(dev_hdl handle);

/**
 * @brief Get the volume dosed since the last dosing command.
 * @param handle Pump device handle
 * @return Dosed volume in configured volume unit
 */
double LCP_GetDosedVolume(dev_hdl handle);

/**
 * @brief Get the maximum possible flow rate for the current syringe.
 * @param handle Pump device handle
 * @return Maximum flow rate in configured flow unit
 */
double LCP_GetFlowRateMax(dev_hdl handle);

/**
 * @brief Get the current actual flow rate.
 * @param handle Pump device handle
 * @return Current flow rate in configured flow unit
 */
double LCP_GetFlowIs(dev_hdl handle);

/**
 * @brief Get the target flow rate of the current dosing command.
 * @param handle Pump device handle
 * @return Target flow rate in configured flow unit
 */
double LCP_GetTargetFlowRate(dev_hdl handle);

/**
 * @brief Check if the pump is currently dosing (moving).
 * @param handle Pump device handle
 * @return 1 if pumping, 0 if idle
 */
int LCP_IsPumping(dev_hdl handle);

/**
 * @brief Check if the pump is in a fault/error condition.
 * @param handle Pump device handle
 * @return 1 if in fault state, 0 if OK
 */
int LCP_IsInFaultState(dev_hdl handle);

/**
 * @brief Check if the syringe calibration is complete.
 * @param handle Pump device handle
 * @return 1 if calibrated, 0 if not
 */
int LCP_IsCalibrationFinished(dev_hdl handle);

/**
 * @brief Get the maximum volume the syringe can hold.
 * @param handle Pump device handle
 * @return Maximum volume in configured volume unit
 */
double LCP_GetVolumeMax(dev_hdl handle);
```

### 3.5 Valve Control (LCV_*)

Optional rotary valve for multi-port switching.

```c
/**
 * @brief Switch valve to a specific position.
 * @param handle   Valve device handle
 * @param position Target valve position (0-based index)
 * @return Error code
 */
long LCV_SwitchValveToPosition(dev_hdl handle, int position);

/**
 * @brief Get the number of valve positions.
 * @param handle Valve device handle
 * @return Number of positions (e.g., 6 for a 6-port valve)
 */
int LCV_NumberOfValvePositions(dev_hdl handle);

/**
 * @brief Get the current valve position.
 * @param handle Valve device handle
 * @return Current position (0-based)
 */
int LCV_ActualValvePosition(dev_hdl handle);
```

### 3.6 Syringe Calibration

```c
/**
 * @brief Start syringe calibration (reference move).
 *
 * Moves the plunger to both end stops to determine the exact stroke
 * length. Must be performed at least once after syringe replacement.
 *
 * @param handle    Pump device handle
 * @param flow_rate Flow rate for the calibration move
 * @return Error code
 */
long LCP_SyringeCalibration(dev_hdl handle, double flow_rate);

/**
 * @brief Clear the fault state of a pump.
 * @param handle Pump device handle
 * @return Error code
 */
long LCP_ClearFault(dev_hdl handle);

/**
 * @brief Enable/disable the pump drive.
 * @param handle Pump device handle
 * @param enable 1 = enable, 0 = disable
 * @return Error code
 */
long LCP_Enable(dev_hdl handle, int enable);
```

---

## 4. Error Model

### Error Codes

All `LCB_*` and `LCP_*` functions return a `long` error code:

| Code | Constant | Meaning |
|------|----------|---------|
| 0 | `ERR_NOERR` | Success |
| -1 | `ERR_PERM` | Operation not permitted |
| -2 | `ERR_PARAM_RANGE` | Parameter out of valid range |
| -3 | `ERR_DEV_NOT_FOUND` | Device not found on the bus |
| -4 | `ERR_NOT_OPEN` | Bus not opened |
| -5 | `ERR_NOT_STARTED` | Bus not started |
| -6 | `ERR_TIMEOUT` | Communication timeout |
| -7 | `ERR_COMM` | General communication error |
| -8 | `ERR_NOT_CALIBRATED` | Syringe not calibrated |
| -9 | `ERR_FAULT` | Device in fault state |
| -10 | `ERR_OVERLOAD` | Motor overload / mechanical stall |
| -11 | `ERR_SYRINGE_EMPTY` | Syringe empty during dispense |
| -12 | `ERR_SYRINGE_FULL` | Syringe full during aspirate |

### Error Handling Pattern

```c
long err = LCP_Dispense(pump, 100.0, 5.0);
if (err != ERR_NOERR) {
  char msg[256];
  LCB_GetLastErrorMsg(msg, sizeof(msg));
  // Log error: msg contains human-readable description
}
```

### Error Recovery

| Error | Recovery Action |
|-------|----------------|
| `ERR_TIMEOUT` | Retry after 500ms, max 3 attempts |
| `ERR_COMM` | Re-open bus (LCB_Close → LCB_Open → LCB_Start) |
| `ERR_FAULT` | Call LCP_ClearFault(), then LCP_Enable(1) |
| `ERR_OVERLOAD` | Stop pump, warn user, check for mechanical obstruction |
| `ERR_SYRINGE_EMPTY` | Stop pump, prompt user to refill |
| `ERR_NOT_CALIBRATED` | Run LCP_SyringeCalibration() before first use |

---

## 5. Units & Conversions

### SDK Unit Enums

```c
// SI Prefix
enum {
  NANO  = -9,   // 10^-9
  MICRO = -6,   // 10^-6
  MILLI = -3,   // 10^-3
  UNIT  =  0    // 10^0
};

// Volume unit
enum {
  UNIT_LITRE = 68  // L (combine with prefix for nL, µL, mL)
};

// Time unit (for flow rate denominator)
enum {
  PER_SECOND = 1,
  PER_MINUTE = 60
};
```

### Unit Conversion Table

| MWA Interface | Unit | SDK Configuration | Conversion Factor |
|---------------|------|-------------------|-------------------|
| `setFlowRate(double)` | µL/min | `LCP_SetFlowUnit(h, MICRO, UNIT_LITRE, PER_MINUTE)` | 1:1 (none) |
| `setTargetVolume(double)` | µL | `LCP_SetVolumeUnit(h, MICRO, UNIT_LITRE)` | 1:1 (none) |
| `currentPosition()` | µL | `LCP_GetFillLevel()` returns configured unit | 1:1 (none) |
| `flowRate()` | µL/min | `LCP_GetFlowIs()` returns configured unit | 1:1 (none) |

**Strategy:** Configure the SDK to use µL and µL/min immediately after bus start.
This eliminates all unit conversions at runtime.

---

## 6. Syringe Specifications

Common syringes used with Cetoni Nemesys (Hamilton or ILS glass syringes):

| Nominal Volume | Inner Diameter (mm) | Stroke (mm) | Max Flow Rate (µL/min) | Min Flow Rate (µL/min) |
|---------------|--------------------|--------------|-----------------------|-----------------------|
| 10 µL | 0.46 | 60.0 | 7.5 | 0.00024 |
| 25 µL | 0.73 | 60.0 | 18.8 | 0.00060 |
| 50 µL | 1.03 | 60.0 | 37.5 | 0.0012 |
| 100 µL | 1.46 | 60.0 | 75.0 | 0.0024 |
| 250 µL | 2.30 | 60.0 | 187.5 | 0.0060 |
| 500 µL | 3.26 | 60.0 | 375.0 | 0.012 |
| 1000 µL (1 mL) | 4.61 | 60.0 | 750.0 | 0.024 |
| 2500 µL (2.5 mL) | 7.29 | 60.0 | 1875.0 | 0.060 |
| 5000 µL (5 mL) | 10.30 | 60.0 | 3750.0 | 0.12 |
| 25000 µL (25 mL) | 23.03 | 60.0 | 18750.0 | 0.60 |

**Flow rate limits** depend on the stepper motor specifications:
- Max flow rate ∝ (inner_diameter)² (cross-sectional area × max plunger speed)
- Min flow rate ∝ (inner_diameter)² × min_step_rate
- The values above are approximate for the Nemesys S module

### Volume Calculation

```
volume = π × (inner_diameter / 2)² × stroke
```

For a 250 µL syringe: π × (2.30/2)² × 60 = π × 1.3225 × 60 ≈ 249.3 µL ✓

---

## 7. Typical Workflows

### 7.1 Initialization

```c
// 1. Open the device bus
long err = LCB_Open("C:/Cetoni/Config/MySetup");
if (err != ERR_NOERR) { /* handle error */ }

// 2. Start bus communication
err = LCB_Start();
if (err != ERR_NOERR) { /* handle error */ }

// 3. Look up pump by name
dev_hdl pump;
err = LCB_LookupPumpByName("neMESYS1_Pump", &pump);
if (err != ERR_NOERR) { /* handle error */ }

// 4. Configure syringe (250 µL Hamilton)
err = LCP_SetSyringeParam(pump, 2.30, 60.0);

// 5. Set units to µL and µL/min
err = LCP_SetVolumeUnit(pump, MICRO, UNIT_LITRE);
err = LCP_SetFlowUnit(pump, MICRO, UNIT_LITRE, PER_MINUTE);

// 6. Enable pump drive
err = LCP_Enable(pump, 1);

// 7. Calibrate if not already done
if (!LCP_IsCalibrationFinished(pump)) {
  err = LCP_SyringeCalibration(pump, 50.0);  // 50 µL/min
  while (!LCP_IsCalibrationFinished(pump)) {
    sleep_ms(100);
  }
}

// Ready for operations
```

### 7.2 Dispensing

```c
// Set up: dispense 100 µL at 5 µL/min
double target_volume = 100.0;  // µL
double flow_rate = 5.0;        // µL/min

// Start dispensing
err = LCP_Dispense(pump, target_volume, flow_rate);
if (err != ERR_NOERR) { /* handle error */ }

// Poll status (from CommandQueue worker thread)
while (LCP_IsPumping(pump)) {
  double dosed = LCP_GetDosedVolume(pump);
  double fill  = LCP_GetFillLevel(pump);
  double flow  = LCP_GetFlowIs(pump);

  // Report progress: dosed / target_volume
  emit_position_update(fill);

  sleep_ms(100);  // Poll every 100ms
}

// Dispensing complete (or error)
if (LCP_IsInFaultState(pump)) {
  // Handle fault (syringe empty, overload, etc.)
} else {
  // Success: dispensed == target_volume
}
```

### 7.3 Refilling

```c
// Aspirate to refill syringe to max capacity
double max_volume = LCP_GetVolumeMax(pump);
double refill_rate = 50.0;  // µL/min (moderate speed)

err = LCP_Aspirate(pump, max_volume, refill_rate);
if (err != ERR_NOERR) { /* handle error */ }

// Wait for refill
while (LCP_IsPumping(pump)) {
  double fill = LCP_GetFillLevel(pump);
  emit_position_update(fill);
  sleep_ms(100);
}
// Syringe now full
```

### 7.4 Emergency Stop

```c
// Immediate stop -- always works, even in fault state
LCP_StopPumping(pump);
```

### 7.5 Shutdown

```c
LCP_StopPumping(pump);
LCP_Enable(pump, 0);
LCB_Stop();
LCB_Close();
```

---

## 8. Platform Availability

| Platform | QmixSDK C API | Cetoni Elements | Notes |
|----------|---------------|-----------------|-------|
| **Windows x64** | **Full support** | Available | Primary development platform |
| **Windows x86** | Available | Available | Legacy |
| **macOS** | **Not available** | Not available | SDK has no macOS build |
| **Linux x64** | **Available** | Not available | SDK available since ~2022 |

### MWA Strategy

```cmake
# Platform-conditional compilation
if(WIN32 OR (UNIX AND NOT APPLE))
  find_package(QmixSDK)
  if(QmixSDK_FOUND)
    target_sources(mwa_hardware PRIVATE pump/pump_controller.cpp)
    target_link_libraries(mwa_hardware PRIVATE QmixSDK::QmixSDK)
    target_compile_definitions(mwa_hardware PRIVATE MWA_HAS_QMIXSDK)
  endif()
endif()

# On macOS or when SDK not found: only MockPumpController available
```

The GUI pump panel works with `PumpControllerInterface*` -- it doesn't know or
care whether the concrete class is `PumpController` (real) or `MockPumpController`.
On macOS, the device manager registers a mock and disables the "real hardware"
option.

---

## 9. CMake Integration

### FindQmixSDK.cmake

```cmake
# FindQmixSDK.cmake -- Locate the Cetoni QmixSDK
#
# Sets:
#   QmixSDK_FOUND         - TRUE if SDK found
#   QmixSDK_INCLUDE_DIRS  - Header paths
#   QmixSDK_LIBRARIES     - Libraries to link
#
# Hints:
#   QMIXSDK_ROOT environment variable or CMake variable

if(APPLE)
  # QmixSDK not available on macOS
  set(QmixSDK_FOUND FALSE)
  return()
endif()

# Search paths
set(_search_paths
  "$ENV{QMIXSDK_ROOT}"
  "${QMIXSDK_ROOT}"
  "C:/QmixSDK"
  "C:/Program Files/CETONI/QmixSDK"
  "C:/Program Files (x86)/CETONI/QmixSDK"
  "/opt/cetoni/qmixsdk"
  "$ENV{HOME}/QmixSDK"
)

# Find header
find_path(QmixSDK_INCLUDE_DIR
  NAMES lcp.h
  PATHS ${_search_paths}
  PATH_SUFFIXES include lib/qmixsdk
)

# Find library
find_library(QmixSDK_LIBRARY
  NAMES qmixsdk lcp
  PATHS ${_search_paths}
  PATH_SUFFIXES lib lib64
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(QmixSDK
  DEFAULT_MSG
  QmixSDK_INCLUDE_DIR
  QmixSDK_LIBRARY
)

if(QmixSDK_FOUND)
  set(QmixSDK_INCLUDE_DIRS ${QmixSDK_INCLUDE_DIR})
  set(QmixSDK_LIBRARIES ${QmixSDK_LIBRARY})

  if(NOT TARGET QmixSDK::QmixSDK)
    add_library(QmixSDK::QmixSDK UNKNOWN IMPORTED)
    set_target_properties(QmixSDK::QmixSDK PROPERTIES
      IMPORTED_LOCATION "${QmixSDK_LIBRARY}"
      INTERFACE_INCLUDE_DIRECTORIES "${QmixSDK_INCLUDE_DIR}"
    )
  endif()
endif()

mark_as_advanced(QmixSDK_INCLUDE_DIR QmixSDK_LIBRARY)
```

---

## 10. Mock Data Patterns

### 10.1 Dispensing Simulation

For `MockPumpController::startInfusion()` with realistic timing:

**Parameters:**
- Syringe: 250 µL (default)
- Flow rate: 5.0 µL/min
- Target volume: 100 µL

**Expected behavior:**
- Duration: 100 µL / 5.0 µL/min = 20 minutes
- Position update interval: 100 ms
- Volume per tick: 5.0 / 60.0 / 10.0 = 0.00833 µL per tick (too small for 20-min real-time)

**Accelerated simulation** (for usability):
- Use a time acceleration factor (e.g., 100x)
- Volume per tick at 100x: 0.833 µL per 100ms tick
- 100 µL / 0.833 = ~120 ticks = ~12 seconds simulated dispensing

```
Time (s)  Fill Level (µL)  Dosed Volume (µL)  Signal
0.0       250.0            0.0                infusionStarted()
0.1       249.2            0.8                positionChanged(249.2)
0.2       248.3            1.7                positionChanged(248.3)
...
12.0      150.0            100.0              positionChanged(150.0)
12.0      150.0            100.0              infusionStopped()
```

### 10.2 Refill Simulation

**Parameters:**
- Refill flow rate: 50 µL/min (fixed, not user-configurable)
- Syringe capacity: 250 µL
- Starting fill level: 150 µL (after previous dispense)
- Volume to aspirate: 100 µL

**At 100x acceleration:**
- Duration: 100 / 50 = 2 minutes real → 1.2 seconds simulated
- Fill level increases from 150 → 250 µL

### 10.3 Error Scenarios

| Scenario | Trigger | Response |
|----------|---------|----------|
| Syringe empty | `fill_level` reaches 0 during dispense | `errorOccurred("Syringe empty")`, auto-stop |
| Syringe full | `fill_level` reaches max during aspirate | Auto-stop (normal refill completion) |
| Over-pressure | Simulated: random 1-in-20 dispense | `errorOccurred("Mechanical stall: check for obstruction")` |
| Bus disconnect | `setSimulateError(true)` | `errorOccurred("Bus communication lost")`, state → kError |

### 10.4 Flow Rate Validation

```
max_flow_rate = kMaxFlowRates[syringe_size_index]

if (flow_rate < 0.001) → errorOccurred("Flow rate too low")
if (flow_rate > max_flow_rate) → errorOccurred("Flow rate exceeds maximum for current syringe")
```

---

## 11. MWA Interface Mapping

| MWA Method | QmixSDK Equivalent | Direction | Notes |
|------------|--------------------|-----------|-------|
| `connectDevice()` | `LCB_Open()` → `LCB_Start()` → `LCB_LookupPumpByName()` → `LCP_SetSyringeParam()` → `LCP_SetVolumeUnit()` → `LCP_SetFlowUnit()` → `LCP_Enable()` | GUI → HW | Multi-step init sequence |
| `disconnectDevice()` | `LCP_StopPumping()` → `LCP_Enable(0)` → `LCB_Stop()` → `LCB_Close()` | GUI → HW | Graceful shutdown |
| `setFlowRate(µL/min)` | Store value; used in next `LCP_Dispense()` call | GUI → cache | SDK takes rate per-command |
| `setTargetVolume(µL)` | Store value; used in next `LCP_Dispense()` call | GUI → cache | SDK takes volume per-command |
| `startInfusion()` | `LCP_Dispense(target_volume, flow_rate)` + start poll timer | GUI → HW | Combines cached rate + volume |
| `stopInfusion()` | `LCP_StopPumping()` + stop poll timer | GUI → HW | Immediate stop |
| `refill()` | `LCP_Aspirate(max_volume, refill_rate)` + start poll timer | GUI → HW | Aspirate to full |
| `flowRate()` | Return cached value (or `LCP_GetFlowIs()`) | HW → GUI | Thread-safe cache read |
| `targetVolume()` | Return cached value | cache → GUI | |
| `currentPosition()` | `LCP_GetFillLevel()` | HW → GUI | Polled via timer |
| `isInfusing()` | `LCP_IsPumping()` | HW → GUI | Polled via timer |
| `flowRateChanged()` | Emit after `LCP_GetFlowIs()` differs | HW → GUI | Signal |
| `positionChanged()` | Emit after `LCP_GetFillLevel()` poll | HW → GUI | Every 100ms during dosing |
| `infusionStarted()` | Emit after `LCP_Dispense()` succeeds | HW → GUI | Signal |
| `infusionStopped()` | Emit when `LCP_IsPumping()` → false | HW → GUI | Signal |

---

## 12. References

1. [Cetoni QmixSDK Online Documentation](https://cetoni.com/downloads/manuals/CETONI_SDK/) --
   Official SDK API reference (C API headers, function signatures, examples)
2. [Cetoni Nemesys Product Page](https://cetoni.com/products/nemesys/) --
   Syringe pump specifications and datasheets
3. [Cetoni SDK Python (GitHub)](https://github.com/CETONI-Software/sila_cetoni_pumps) --
   SiLA2-based Python SDK showing modern API patterns
4. [QmixSDK C API Example: capi_nemesys_test.cpp](https://cetoni.com/downloads/manuals/CETONI_SDK/capi_nemesys_test_8cpp-example.html) --
   Official C API usage example
5. [Hamilton Syringe Catalog](https://www.hamiltoncompany.com/laboratory-products/syringes) --
   Syringe dimensions (inner diameter, stroke) for specification table
6. [Cetoni Elements Software](https://cetoni.com/products/software/) --
   Creates device configuration files used by `LCB_Open()`

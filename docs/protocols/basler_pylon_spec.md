# Basler Pylon SDK Specification -- MWA Project

> **Purpose:** Reference document for implementing the Basler camera driver
> (`BaslerCameraController`) and enhancing the mock camera
> (`MockCameraController`) in the MWA application.
>
> **SDK version target:** Pylon 7.x (C++ API)
> **Camera models:** Basler ace U / ace 2 series (USB3 Vision, GigE Vision)

---

## 1. Overview

The Basler Pylon Camera Software Suite provides a C++ SDK for controlling
Basler industrial cameras. The SDK wraps the GenICam standard and exposes a
high-level `CInstantCamera` class that handles device discovery, connection,
parameter configuration, and image acquisition. All camera parameters
(exposure, gain, ROI, pixel format, etc.) are accessed through the GenICam
node map.

Key headers:

```cpp
#include <pylon/PylonIncludes.h>      // Core SDK types
#include <pylon/BaslerUniversalInstantCamera.h>  // CBaslerUniversalInstantCamera
#include <pylon/ImageFormatConverter.h>           // CImageFormatConverter
```

Namespace: `Pylon::` for SDK classes, `GenApi::` for GenICam node access.

---

## 2. SDK Architecture

### 2.1 Global Lifecycle

The Pylon runtime **must** be initialized before any SDK call and terminated
after all cameras are released. Two approaches:

```cpp
// Approach 1: Manual (explicit)
Pylon::PylonInitialize();
// ... use cameras ...
Pylon::PylonTerminate();

// Approach 2: RAII (recommended)
{
  Pylon::PylonAutoInitTerm auto_init_term;
  // ... use cameras ...
}  // PylonTerminate() called automatically
```

**MWA strategy:** Call `PylonInitialize()` once during application startup
(e.g., in `main()` before `QApplication`) and `PylonTerminate()` at shutdown.
Alternatively, hold a `PylonAutoInitTerm` as a static/member in the
application object.

### 2.2 Transport Layer Factory

`CTlFactory` is a singleton that discovers and creates camera device objects.

```cpp
using namespace Pylon;

CTlFactory& factory = CTlFactory::GetInstance();

// Enumerate all attached devices
DeviceInfoList_t devices;
int count = factory.EnumerateDevices(devices);

for (const auto& info : devices) {
  qDebug() << "Model:" << info.GetModelName().c_str();
  qDebug() << "Serial:" << info.GetSerialNumber().c_str();
  // GigE only:
  // qDebug() << "IP:" << info.GetIpAddress().c_str();
}
```

Device creation shortcuts:

| Method | Use case |
|--------|----------|
| `factory.CreateFirstDevice()` | Grab the first available camera |
| `factory.CreateDevice(info)` | Create from a specific `CDeviceInfo` |

### 2.3 CInstantCamera

`CInstantCamera` is the primary high-level camera abstraction. It manages
the device lifecycle, buffer pool, grab engine, and event dispatch.

`CBaslerUniversalInstantCamera` extends `CInstantCamera` with typed
parameter accessors (e.g., `camera.ExposureTime` instead of node map
lookups). **Prefer `CBaslerUniversalInstantCamera` for MWA** -- it provides
IDE auto-completion and compile-time checking of parameter names.

```cpp
// Create and attach to the first available camera
CBaslerUniversalInstantCamera camera(
    CTlFactory::GetInstance().CreateFirstDevice());

std::cout << "Using: "
          << camera.GetDeviceInfo().GetModelName() << std::endl;
```

Key state methods:

| Method | Description |
|--------|-------------|
| `camera.Open()` | Open the device, load the node map |
| `camera.Close()` | Close the device, release resources |
| `camera.IsOpen()` | Returns `true` if the device is open |
| `camera.IsPylonDeviceAttached()` | Returns `true` if a device is attached |
| `camera.GetNodeMap()` | Returns the GenICam node map |
| `camera.GetDeviceInfo()` | Returns `CDeviceInfo` (model, serial, etc.) |

### 2.4 Grab Results

`CGrabResultPtr` is a reference-counted smart pointer to `CGrabResultData`.
When the smart pointer is destroyed or re-assigned, the buffer is
automatically returned to the grab engine's pool.

Key `CGrabResultData` methods:

| Method | Return | Description |
|--------|--------|-------------|
| `GrabSucceeded()` | `bool` | Whether the frame was captured successfully |
| `GetErrorCode()` | `uint32_t` | Error code if grab failed |
| `GetErrorDescription()` | `String` | Human-readable error description |
| `GetWidth()` | `uint32_t` | Image width in pixels |
| `GetHeight()` | `uint32_t` | Image height in pixels |
| `GetOffsetX()` | `uint32_t` | Horizontal ROI offset |
| `GetOffsetY()` | `uint32_t` | Vertical ROI offset |
| `GetPixelType()` | `EPixelType` | Pixel format enum |
| `GetBuffer()` | `const void*` | Raw pixel data pointer |
| `GetBufferSize()` | `size_t` | Buffer size in bytes |
| `GetImageSize()` | `size_t` | Actual image data size |
| `GetStride()` | `size_t` | Bytes per row (incl. padding) |
| `GetPaddingX()` | `size_t` | Extra bytes per row (padding) |
| `GetPaddingY()` | `size_t` | Extra bytes after last row |
| `GetTimeStamp()` | `uint64_t` | Hardware timestamp |
| `GetImageNumber()` | `int64_t` | Sequential frame number |
| `GetNumberOfSkippedImages()` | `int64_t` | Count of dropped frames |

---

## 3. Device Enumeration & Connection

### 3.1 Complete Enumeration + Connection Example

```cpp
#include <pylon/PylonIncludes.h>
#include <pylon/BaslerUniversalInstantCamera.h>

using namespace Pylon;

void connectToCamera(const QString& serial_number) {
  CTlFactory& factory = CTlFactory::GetInstance();

  // 1. Enumerate all devices
  DeviceInfoList_t devices;
  if (factory.EnumerateDevices(devices) == 0) {
    throw std::runtime_error("No Basler cameras found");
  }

  // 2. Find the camera with the requested serial number
  CDeviceInfo target;
  bool found = false;
  for (const auto& info : devices) {
    if (QString::fromStdString(
            std::string(info.GetSerialNumber().c_str()))
        == serial_number) {
      target = info;
      found = true;
      break;
    }
  }
  if (!found) {
    throw std::runtime_error("Camera not found: "
        + serial_number.toStdString());
  }

  // 3. Create and open the camera
  CBaslerUniversalInstantCamera camera(
      factory.CreateDevice(target));
  camera.Open();

  // 4. Read device info
  qDebug() << "Model:" << camera.GetDeviceInfo().GetModelName().c_str();
  qDebug() << "Open:" << camera.IsOpen();

  // ... configure and grab ...

  camera.Close();
}
```

### 3.2 Disconnect & Cleanup

```cpp
// Stop any active grab first
if (camera.IsGrabbing()) {
  camera.StopGrabbing();
}
camera.Close();
// camera.IsPylonDeviceAttached() still true until camera is destroyed
```

---

## 4. Camera Parameters (GenICam)

All parameters are accessed through the GenICam node map. With
`CBaslerUniversalInstantCamera`, typed accessors are available directly
on the camera object.

### 4.1 Exposure

| Aspect | Detail |
|--------|--------|
| Parameter name (SFNC 2.0+) | `ExposureTime` |
| Parameter name (legacy) | `ExposureTimeAbs` |
| Unit | Microseconds (us) |
| Type | `IFloat` (double) |
| Typical range | 29 us -- 10,000,000 us (10 s) |
| Auto | `ExposureAuto` (`Off`, `Once`, `Continuous`) |
| Mode | `ExposureMode` must be `Timed` for manual control |

```cpp
// Using CBaslerUniversalInstantCamera typed accessors
camera.ExposureAuto.SetValue(ExposureAuto_Off);
camera.ExposureTime.SetValue(5000.0);  // 5000 us = 5 ms
double current = camera.ExposureTime.GetValue();
double min_exp = camera.ExposureTime.GetMin();
double max_exp = camera.ExposureTime.GetMax();

// Using generic GenApi node map (works with any CInstantCamera)
GenApi::CFloatPtr exposure_node(
    camera.GetNodeMap().GetNode("ExposureTime"));
if (exposure_node.IsValid()) {
  exposure_node->SetValue(5000.0);
}
```

### 4.2 Gain

| Aspect | Detail |
|--------|--------|
| Parameter name (SFNC 2.0+) | `Gain` |
| Parameter name (legacy) | `GainRaw` (integer, needs conversion) |
| Unit | Decibels (dB) |
| Type | `IFloat` (double) for SFNC 2.0+; `IInteger` for legacy |
| Typical range | 0.0 -- 36.0 dB (model-dependent) |
| Auto | `GainAuto` (`Off`, `Once`, `Continuous`) |
| Selector | `GainSelector` (`All`, `AnalogAll`, `DigitalAll`) |

```cpp
camera.GainAuto.SetValue(GainAuto_Off);
camera.GainSelector.SetValue(GainSelector_All);
camera.Gain.SetValue(6.0);  // 6.0 dB
double current_gain = camera.Gain.GetValue();
```

**Legacy conversion (older GigE models):**

```
Gain_dB = 20 * log10(GainRaw / 136)
```

Modern ace/ace2 cameras accept dB values directly.

### 4.3 Region of Interest

ROI is controlled by four integer parameters. Values must satisfy alignment
constraints (often multiples of 2 or 4, model-dependent).

| Parameter | Type | Description |
|-----------|------|-------------|
| `Width` | `IInteger` | ROI width in pixels |
| `Height` | `IInteger` | ROI height in pixels |
| `OffsetX` | `IInteger` | Horizontal offset from sensor origin |
| `OffsetY` | `IInteger` | Vertical offset from sensor origin |

Constraint: `OffsetX + Width <= SensorWidth` and
`OffsetY + Height <= SensorHeight`.

```cpp
// Set ROI to 640x480 centered on a 2448x2048 sensor
camera.OffsetX.SetValue(0);   // Reset offsets first
camera.OffsetY.SetValue(0);
camera.Width.SetValue(640);
camera.Height.SetValue(480);
camera.OffsetX.SetValue((2448 - 640) / 2);  // 904
camera.OffsetY.SetValue((2048 - 480) / 2);  // 784

// Query current ROI
int w = static_cast<int>(camera.Width.GetValue());
int h = static_cast<int>(camera.Height.GetValue());
int ox = static_cast<int>(camera.OffsetX.GetValue());
int oy = static_cast<int>(camera.OffsetY.GetValue());
```

**Important:** Always reset offsets to 0 before shrinking Width/Height, then
set the new Width/Height, then set the new offsets. Otherwise the constraint
check may fail.

### 4.4 Pixel Format

| Format | Bits/pixel | Bytes/pixel | Description |
|--------|-----------|-------------|-------------|
| `Mono8` | 8 | 1 | 8-bit grayscale (most common for microscopy) |
| `Mono10` | 10 | 2 (padded) | 10-bit grayscale, padded to 16 bits |
| `Mono10p` | 10 | 1.25 (packed) | 10-bit packed |
| `Mono12` | 12 | 2 (padded) | 12-bit grayscale, padded to 16 bits |
| `Mono12p` | 12 | 1.5 (packed) | 12-bit packed |
| `BayerRG8` | 8 | 1 | Bayer color (color cameras) |
| `BayerBG8` | 8 | 1 | Bayer color variant |
| `RGB8` | 24 | 3 | 8-bit RGB (after debayering) |
| `BGR8` | 24 | 3 | 8-bit BGR |
| `YCbCr422_8` | 16 | 2 | YCbCr 4:2:2 |

```cpp
camera.PixelFormat.SetValue(PixelFormat_Mono8);
```

**MWA default:** `Mono8` for monochrome microscopy imaging. Color cameras
should use `BayerRG8` or `BayerBG8` and convert via `CImageFormatConverter`.

### 4.5 Frame Rate

| Parameter | Description |
|-----------|-------------|
| `AcquisitionFrameRateEnable` | Must be `true` to control frame rate |
| `AcquisitionFrameRate` | Target frame rate in fps |
| `ResultingFrameRate` | Read-only, actual achievable fps |

The resulting frame rate is bounded by:
`max_fps = 1,000,000 / ExposureTime_us` (plus readout overhead).

```cpp
camera.AcquisitionFrameRateEnable.SetValue(true);
camera.AcquisitionFrameRate.SetValue(30.0);  // Target 30 fps
double actual = camera.ResultingFrameRate.GetValue();
```

---

## 5. Image Acquisition Modes

### 5.1 Single Frame

The simplest acquisition mode. Internally calls `StartGrabbing(1)` +
`RetrieveResult()` + `StopGrabbing()`.

```cpp
CGrabResultPtr grab_result;
// GrabOne(timeout_ms, result, timeout_handling)
camera.GrabOne(5000, grab_result, TimeoutHandling_ThrowException);

if (grab_result->GrabSucceeded()) {
  const uint8_t* buffer =
      static_cast<const uint8_t*>(grab_result->GetBuffer());
  int width = grab_result->GetWidth();
  int height = grab_result->GetHeight();
  // Process image...
} else {
  qWarning() << "Grab failed:"
             << grab_result->GetErrorDescription().c_str();
}
```

### 5.2 Continuous Capture

For live preview. Uses `GrabStrategy_LatestImageOnly` to always get the
most recent frame (drops older frames if processing is slow).

```cpp
camera.StartGrabbing(GrabStrategy_LatestImageOnly);

while (camera.IsGrabbing()) {
  CGrabResultPtr grab_result;
  // Blocks up to 5000 ms for next frame
  camera.RetrieveResult(5000, grab_result,
                        TimeoutHandling_ThrowException);

  if (grab_result->GrabSucceeded()) {
    // Convert and emit to GUI...
  }

  if (should_stop) {
    camera.StopGrabbing();
  }
}
```

**Grab strategies:**

| Strategy | Behavior |
|----------|----------|
| `GrabStrategy_OneByOne` | Process every frame in order (default) |
| `GrabStrategy_LatestImageOnly` | Keep only the newest frame, drop older |
| `GrabStrategy_LatestImages` | Keep N latest frames (configurable) |

**MWA usage:** `GrabStrategy_LatestImageOnly` for continuous capture
(live preview), `GrabStrategy_OneByOne` for batch capture (no drops).

### 5.3 Software-Triggered Batch

For precisely timed batch captures. The camera waits for a software trigger
before capturing each frame.

```cpp
// 1. Configure software triggering
camera.TriggerMode.SetValue(TriggerMode_On);
camera.TriggerSource.SetValue(TriggerSource_Software);

// 2. Start grabbing (will wait for triggers)
camera.StartGrabbing(count, GrabStrategy_OneByOne);

for (int i = 0; i < count; ++i) {
  // 3. Wait until camera is ready, then fire trigger
  camera.WaitForFrameTriggerReady(5000,
      TimeoutHandling_ThrowException);
  camera.ExecuteSoftwareTrigger();

  // 4. Retrieve the triggered frame
  CGrabResultPtr grab_result;
  camera.RetrieveResult(5000, grab_result,
                        TimeoutHandling_ThrowException);

  if (grab_result->GrabSucceeded()) {
    // Store frame in batch...
  }

  // 5. Wait for the inter-frame interval
  if (i < count - 1) {
    QThread::msleep(interval_ms);
  }
}

// 6. Restore free-run mode
camera.TriggerMode.SetValue(TriggerMode_Off);
camera.StopGrabbing();
```

**Helper class:** `CSoftwareTriggerConfiguration` can be registered with
the camera to automatically configure software triggering:

```cpp
camera.RegisterConfiguration(
    new CSoftwareTriggerConfiguration,
    RegistrationMode_ReplaceAll,
    Cleanup_Delete);
```

### 5.4 Event-Driven Capture

For asynchronous processing without a polling loop. Subclass
`CImageEventHandler` and register it with the camera.

```cpp
class MwaImageHandler : public Pylon::CImageEventHandler {
 public:
  void OnImageGrabbed(Pylon::CInstantCamera& camera,
                      const Pylon::CGrabResultPtr& grab_result)
      override {
    if (grab_result->GrabSucceeded()) {
      // Convert to QImage and emit signal
      // NOTE: This is called from the grab loop thread,
      // not the GUI thread. Use queued signal/slot.
    }
  }
};

// Register the handler
MwaImageHandler* handler = new MwaImageHandler();
camera.RegisterImageEventHandler(
    handler,
    RegistrationMode_Append,
    Cleanup_Delete);

// Start with internal grab loop thread
camera.StartGrabbing(GrabStrategy_LatestImageOnly,
                     GrabLoop_ProvidedByInstantCamera);

// Camera now calls OnImageGrabbed() asynchronously.
// No manual RetrieveResult() loop needed.
```

**`GrabLoop` options:**

| Option | Description |
|--------|-------------|
| `GrabLoop_ProvidedByUser` | User calls `RetrieveResult()` in a loop |
| `GrabLoop_ProvidedByInstantCamera` | SDK runs its own grab thread, calls event handlers |

**MWA strategy:** Use `GrabLoop_ProvidedByInstantCamera` with a custom
`CImageEventHandler` that converts frames and emits a Qt signal via
`QMetaObject::invokeMethod()` to cross the thread boundary to the GUI.

---

## 6. Image Conversion to QImage

### 6.1 CImageFormatConverter

The SDK provides `CImageFormatConverter` for pixel format conversion.

```cpp
Pylon::CImageFormatConverter converter;
converter.OutputPixelFormat = Pylon::PixelType_Mono8;
// Or for color: converter.OutputPixelFormat = Pylon::PixelType_RGB8packed;

Pylon::CPylonImage pylon_image;
converter.Convert(pylon_image, grab_result);

const uint8_t* buffer =
    static_cast<const uint8_t*>(pylon_image.GetBuffer());
int width = pylon_image.GetWidth();
int height = pylon_image.GetHeight();
```

Key `CImageFormatConverter` properties:

| Property | Description |
|----------|-------------|
| `OutputPixelFormat` | Target pixel type (`PixelType_Mono8`, `PixelType_RGB8packed`, etc.) |
| `OutputPaddingX` | Extra bytes per row in output |
| `Gamma` | Gamma correction value |
| `MonoConversionMethod` | How color is converted to mono |

### 6.2 Mono8 to QImage (Direct Copy)

When the camera outputs `Mono8`, no format conversion is needed. The raw
buffer can be copied directly into a `QImage::Format_Grayscale8`.

```cpp
QImage pylonToQImage(const Pylon::CGrabResultPtr& grab_result) {
  const int width = grab_result->GetWidth();
  const int height = grab_result->GetHeight();
  const size_t stride = grab_result->GetStride();
  const auto* src = static_cast<const uint8_t*>(
      grab_result->GetBuffer());

  QImage image(width, height, QImage::Format_Grayscale8);

  if (stride == static_cast<size_t>(image.bytesPerLine())) {
    // No padding difference -- bulk copy
    memcpy(image.bits(), src, height * stride);
  } else {
    // Row-by-row copy to handle stride mismatch
    for (int row = 0; row < height; ++row) {
      memcpy(image.scanLine(row),
             src + row * stride,
             width);  // copy only pixel data, not padding
    }
  }

  return image;
}
```

### 6.3 RGB8 to QImage

```cpp
QImage pylonRgbToQImage(const Pylon::CGrabResultPtr& grab_result,
                        Pylon::CImageFormatConverter& converter) {
  Pylon::CPylonImage pylon_image;
  converter.OutputPixelFormat = Pylon::PixelType_RGB8packed;
  converter.Convert(pylon_image, grab_result);

  const int width = pylon_image.GetWidth();
  const int height = pylon_image.GetHeight();
  const auto* src = static_cast<const uint8_t*>(
      pylon_image.GetBuffer());

  // QImage takes ownership of a copy
  QImage image(src, width, height,
               width * 3,  // bytes per line
               QImage::Format_RGB888);
  return image.copy();  // Deep copy -- pylon_image buffer is temporary
}
```

### 6.4 Format Mapping Table

| Pylon Pixel Type | QImage Format | Notes |
|------------------|---------------|-------|
| `PixelType_Mono8` | `Format_Grayscale8` | Direct copy, most efficient |
| `PixelType_RGB8packed` | `Format_RGB888` | 3 bytes/pixel |
| `PixelType_BGR8packed` | `Format_RGB888` | Needs R/B swap or convert first |
| `PixelType_Mono10/12` | Convert to `Mono8` first | Use `CImageFormatConverter` |
| `PixelType_BayerRG8` | Convert to `RGB8packed` first | Use `CImageFormatConverter` |

---

## 7. Unit Conversions (MWA <-> Pylon)

The MWA interface uses user-friendly units. The Pylon SDK uses GenICam
standard units. Conversions are needed at the driver boundary.

| Parameter | MWA Unit | Pylon Unit | Conversion |
|-----------|----------|------------|------------|
| Exposure | milliseconds (`double`) | microseconds (`double`) | `pylon_us = mwa_ms * 1000.0` |
| Gain | multiplier (`double`, 1.0 = unity) | decibels (`double`) | `pylon_dB = 20.0 * log10(mwa_gain)` |
| ROI | `QRect` (x, y, w, h) | `OffsetX`, `OffsetY`, `Width`, `Height` (int64) | Direct mapping (pixel units) |
| Frame rate | N/A (not in interface) | fps (`double`) | N/A |

### 7.1 Exposure Conversion

```cpp
// MWA -> Pylon
void BaslerCameraController::setExposure(double ms) {
  double us = ms * 1000.0;
  camera_.ExposureTime.SetValue(us);
  exposure_ms_ = ms;
  emit exposureChanged(ms);
}

// Pylon -> MWA
double BaslerCameraController::exposure() const {
  double us = camera_.ExposureTime.GetValue();
  return us / 1000.0;  // Convert back to ms
}
```

### 7.2 Gain Conversion

The MWA interface uses a linear multiplier (1.0 = unity, 2.0 = double).
Pylon uses decibels. The conversion is:

```
gain_dB = 20 * log10(gain_multiplier)
gain_multiplier = pow(10, gain_dB / 20)
```

| MWA Multiplier | Pylon dB |
|----------------|----------|
| 0.5 | -6.02 |
| 1.0 | 0.0 |
| 2.0 | 6.02 |
| 4.0 | 12.04 |
| 10.0 | 20.0 |

```cpp
// MWA -> Pylon
void BaslerCameraController::setGain(double gain) {
  double db = 20.0 * std::log10(std::max(gain, 0.001));
  db = std::clamp(db, camera_.Gain.GetMin(), camera_.Gain.GetMax());
  camera_.Gain.SetValue(db);
  gain_ = gain;
  emit gainChanged(gain);
}

// Pylon -> MWA
double BaslerCameraController::gain() const {
  double db = camera_.Gain.GetValue();
  return std::pow(10.0, db / 20.0);
}
```

### 7.3 ROI Conversion

Direct pixel-unit mapping. No scaling needed.

```cpp
// MWA -> Pylon (order matters!)
void BaslerCameraController::setRoi(const QRect& roi) {
  camera_.OffsetX.SetValue(0);  // Reset first
  camera_.OffsetY.SetValue(0);
  camera_.Width.SetValue(roi.width());
  camera_.Height.SetValue(roi.height());
  camera_.OffsetX.SetValue(roi.x());
  camera_.OffsetY.SetValue(roi.y());
  roi_ = roi;
}

// Pylon -> MWA
QRect BaslerCameraController::roi() const {
  return QRect(
      static_cast<int>(camera_.OffsetX.GetValue()),
      static_cast<int>(camera_.OffsetY.GetValue()),
      static_cast<int>(camera_.Width.GetValue()),
      static_cast<int>(camera_.Height.GetValue()));
}
```

---

## 8. Error Handling

### 8.1 Exception Hierarchy

All Pylon exceptions inherit from `Pylon::GenericException` (which itself
inherits from `GenICam::GenericException`). Key exception types:

| Exception | Cause |
|-----------|-------|
| `GenericException` | Base class for all Pylon errors |
| `TimeoutException` | `RetrieveResult()` or `GrabOne()` timed out |
| `RuntimeException` | General runtime errors (device lost, etc.) |
| `LogicalErrorException` | Programming error (invalid parameter, etc.) |
| `AccessException` | Parameter access denied (e.g., read-only) |

### 8.2 Error Handling Pattern

```cpp
try {
  camera.Open();
  camera.GrabOne(5000, grab_result, TimeoutHandling_ThrowException);
} catch (const Pylon::TimeoutException& e) {
  qWarning() << "Grab timed out:" << e.GetDescription();
  emit errorOccurred("Camera grab timed out");
} catch (const Pylon::GenericException& e) {
  qCritical() << "Pylon error:" << e.GetDescription();
  emit errorOccurred(QString("Camera error: %1")
      .arg(e.GetDescription()));
}
```

### 8.3 Per-Frame Error Checking

Always check `GrabSucceeded()` before accessing pixel data:

```cpp
if (grab_result->GrabSucceeded()) {
  // Safe to access GetBuffer(), GetWidth(), etc.
} else {
  qWarning() << "Frame error:"
             << grab_result->GetErrorCode() << "--"
             << grab_result->GetErrorDescription().c_str();
}
```

### 8.4 Timeout Handling Options

`RetrieveResult()` accepts a `ETimeoutHandling` parameter:

| Option | Behavior |
|--------|----------|
| `TimeoutHandling_ThrowException` | Throws `TimeoutException` on timeout |
| `TimeoutHandling_Return` | Returns `false` on timeout (no exception) |

**MWA recommendation:** Use `TimeoutHandling_ThrowException` for
`GrabOne()` (single frame) and `TimeoutHandling_Return` for continuous
capture loops where timeouts are recoverable.

---

## 9. CMake Integration

### 9.1 macOS

Pylon installs as a framework at `/Library/Frameworks/pylon.framework/`.

```cmake
# FindPylon.cmake (macOS section)
if(APPLE)
  set(PYLON_FRAMEWORK "/Library/Frameworks/pylon.framework")
  set(PYLON_INCLUDE_DIRS
      "${PYLON_FRAMEWORK}/Headers"
      "${PYLON_FRAMEWORK}/Headers/GenICam")
  set(PYLON_LIBRARY_DIR "${PYLON_FRAMEWORK}/Libraries")
  set(PYLON_LIBRARIES
      pylonbase
      pylonutility
      GenApi_gcc_v3_1_Basler_pylon
      GCBase_gcc_v3_1_Basler_pylon)
  link_directories(${PYLON_LIBRARY_DIR})
endif()
```

Runtime: set `DYLD_LIBRARY_PATH="/Library/Frameworks/pylon.framework/Libraries"`.

### 9.2 Windows

Pylon installs to `C:\Program Files\Basler\pylon 7\`.

```cmake
# FindPylon.cmake (Windows section)
if(WIN32)
  set(PYLON_ROOT "$ENV{PYLON_DEV_DIR}")
  if(NOT PYLON_ROOT)
    set(PYLON_ROOT "C:/Program Files/Basler/pylon 7/Development")
  endif()
  set(PYLON_INCLUDE_DIRS "${PYLON_ROOT}/include")
  if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(PYLON_LIBRARY_DIR "${PYLON_ROOT}/lib/x64")
  else()
    set(PYLON_LIBRARY_DIR "${PYLON_ROOT}/lib/Win32")
  endif()
  set(PYLON_LIBRARIES
      PylonBase_v7_4
      PylonUtility_v7_4
      GCBase_MD_VC141_v3_1_Basler_pylon
      GenApi_MD_VC141_v3_1_Basler_pylon)
  link_directories(${PYLON_LIBRARY_DIR})
endif()
```

### 9.3 Linux (pylon-config)

```cmake
# FindPylon.cmake (Linux section)
if(UNIX AND NOT APPLE)
  set(PYLON_ROOT "$ENV{PYLON_ROOT}")
  if(NOT PYLON_ROOT)
    set(PYLON_ROOT "/opt/pylon")
  endif()
  set(PYLON_CONFIG "${PYLON_ROOT}/bin/pylon-config")
  if(EXISTS ${PYLON_CONFIG})
    execute_process(COMMAND ${PYLON_CONFIG} --cflags-only-I
                    OUTPUT_VARIABLE PYLON_INCLUDE_FLAGS)
    execute_process(COMMAND ${PYLON_CONFIG} --libs
                    OUTPUT_VARIABLE PYLON_LINK_FLAGS)
    # Parse flags into CMake variables...
  endif()
endif()
```

### 9.4 MWA CMakeLists.txt Integration

```cmake
# In the MWA CMakeLists.txt, Pylon is optional (mock always available)
option(MWA_USE_PYLON "Build with Basler Pylon SDK support" OFF)

if(MWA_USE_PYLON)
  list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
  find_package(Pylon REQUIRED)
  target_include_directories(mwa PRIVATE ${PYLON_INCLUDE_DIRS})
  target_link_libraries(mwa PRIVATE ${PYLON_LIBRARIES})
  target_compile_definitions(mwa PRIVATE MWA_HAS_PYLON=1)
endif()
```

In source code:

```cpp
#ifdef MWA_HAS_PYLON
#include "hardware/camera/basler_camera_controller.h"
#else
// Mock-only build
#endif
```

---

## 10. Performance & Buffer Management

### 10.1 Buffer Pool

The `CInstantCamera` maintains an internal buffer pool. The pool size is
controlled by `MaxNumBuffer`:

```cpp
camera.MaxNumBuffer = 10;  // Default is 10
```

Each buffer holds one complete frame:
`buffer_bytes = width * height * bytes_per_pixel + padding`

Example: 2448 x 2048 Mono8 = ~5.0 MB per buffer. 10 buffers = ~50 MB.

### 10.2 Bandwidth Considerations

| Interface | Max Bandwidth | Typical Max FPS (5 MP Mono8) |
|-----------|--------------|------------------------------|
| USB3 Vision | ~380 MB/s | ~75 fps |
| GigE Vision | ~115 MB/s | ~23 fps |

Frame rate is limited by:
1. `1,000,000 / ExposureTime_us` (exposure ceiling)
2. Sensor readout time
3. Interface bandwidth: `bandwidth / (width * height * bpp)`

### 10.3 Stream Grabber Tuning

| Parameter | Description | Default |
|-----------|-------------|---------|
| `MaxNumBuffer` | Number of pre-allocated grab buffers | 10 |
| `MaxTransferSize` (USB3) | Max USB transfer size in bytes | Auto |
| `NumMaxQueuedUrbs` (USB3) | Max USB request blocks | Auto |
| `GevSCPSPacketSize` (GigE) | Jumbo frame packet size | 1500 |
| `GevSCPD` (GigE) | Inter-packet delay | 0 |

**MWA recommendation:** Use defaults for most cases. Increase `MaxNumBuffer`
to 20 if frame drops occur during batch capture.

### 10.4 Memory Budget

For a typical microscopy setup with ROI 640 x 480 Mono8:
- Per frame: 640 * 480 = 307,200 bytes (~300 KB)
- 10 buffers: ~3 MB
- 100-frame batch: ~30 MB

For full sensor 2448 x 2048 Mono8:
- Per frame: ~5.0 MB
- 10 buffers: ~50 MB
- 100-frame batch: ~500 MB (consider streaming to disk)

---

## 11. Typical Camera Specifications

### 11.1 Basler acA2440-75um (USB3, Monochrome)

| Specification | Value |
|---------------|-------|
| Sensor | Sony IMX250, 2/3" CMOS, global shutter |
| Resolution | 2448 x 2048 (5.01 MP) |
| Pixel size | 3.45 x 3.45 um |
| Max frame rate | 75 fps (full), 143 fps @ 2448x1080, 323 fps @ 2448x480 |
| Interface | USB3 Vision |
| Pixel formats | Mono8, Mono10, Mono10p, Mono12, Mono12p |
| Dimensions | 42 x 29 x 29 mm |
| Weight | 90 g |
| Applications | Microscopy, medical imaging, machine vision |

### 11.2 Basler acA1920-40gm (GigE, Monochrome)

| Specification | Value |
|---------------|-------|
| Sensor | Sony IMX249, 1/1.2" CMOS, global shutter |
| Resolution | 1920 x 1200 (2.3 MP) |
| Pixel size | 5.86 x 5.86 um |
| Max frame rate | 42 fps (full) |
| Interface | GigE Vision (PoE supported) |
| Pixel formats | Mono8, Mono10, Mono12, Mono12p |
| Dimensions | 42 x 29 x 29 mm |
| Weight | 90 g |
| Applications | Microscopy, factory automation |

### 11.3 Typical Microscopy Configuration

For MWA microfluidics imaging, a typical setup would be:

| Parameter | Value | Rationale |
|-----------|-------|-----------|
| Pixel format | Mono8 | Sufficient for fluorescence/brightfield |
| ROI | 640 x 480 (center) | Standard microchannel field of view |
| Exposure | 1--100 ms | Depends on illumination and flow speed |
| Gain | 0--12 dB | Keep low to minimize noise |
| Frame rate | 10--30 fps | Live preview; batch at lower rate |
| Trigger | Software | Timed batch captures |

---

## 12. MWA Interface Mapping

Complete mapping from `CameraControllerInterface` to Pylon SDK calls:

| MWA Method | Pylon Implementation |
|------------|---------------------|
| `connectDevice()` | `PylonInitialize()` (if not already), `CTlFactory::EnumerateDevices()`, `CBaslerUniversalInstantCamera(factory.CreateDevice(info))`, `camera.Open()` |
| `disconnectDevice()` | `camera.StopGrabbing()` (if grabbing), `camera.Close()` |
| `deviceName()` | `camera.GetDeviceInfo().GetModelName()` |
| `state()` | Map: not attached -> `kDisconnected`, `Open()` in progress -> `kConnecting`, `IsOpen()` -> `kConnected`, exception -> `kError` |
| `isConnected()` | `camera.IsOpen()` |
| `setExposure(ms)` | `camera.ExposureAuto.SetValue(Off)`, `camera.ExposureTime.SetValue(ms * 1000.0)` |
| `exposure()` | `camera.ExposureTime.GetValue() / 1000.0` |
| `setGain(multiplier)` | `camera.GainAuto.SetValue(Off)`, `camera.Gain.SetValue(20.0 * log10(multiplier))` |
| `gain()` | `pow(10.0, camera.Gain.GetValue() / 20.0)` |
| `setRoi(QRect)` | Reset offsets, set Width/Height, then set OffsetX/OffsetY |
| `roi()` | `QRect(OffsetX, OffsetY, Width, Height)` |
| `grabSingle()` | `camera.GrabOne(5000, result)`, convert to QImage, emit `frameReady()` |
| `startContinuousCapture()` | `camera.StartGrabbing(GrabStrategy_LatestImageOnly, GrabLoop_ProvidedByInstantCamera)` with registered `CImageEventHandler` |
| `stopCapture()` | `camera.StopGrabbing()` |
| `startBatchCapture(count, interval_ms)` | Configure software trigger, `StartGrabbing(count)`, loop: trigger + retrieve + sleep |
| `isCapturing()` | `camera.IsGrabbing()` |
| `lastFrame()` | Return cached `QImage last_frame_` (updated by grab callback) |

### Signal Mapping

| MWA Signal | Trigger |
|------------|---------|
| `stateChanged(DeviceState)` | After `Open()` succeeds/fails, after `Close()` |
| `errorOccurred(QString)` | On `GenericException` catch |
| `frameReady(QImage)` | After each successful grab + conversion |
| `batchComplete()` | After all batch frames captured |
| `exposureChanged(double)` | After `setExposure()` succeeds |
| `gainChanged(double)` | After `setGain()` succeeds |

---

## 13. Mock Frame Generation Strategy

The current `MockCameraController` generates a simple checkerboard pattern.
To better simulate real camera behavior for GUI development and testing,
the mock should generate synthetic microscopy-like images.

### 13.1 Synthetic Microscopy Image Model

A microfluidics microscopy image consists of these visual elements:

1. **Dark background** (intensity ~10--30): The region outside the
   microchannel, representing the chip substrate.
2. **Channel region** (intensity ~40--80): A lighter rectangular band
   running horizontally through the field of view, representing the
   transparent microchannel filled with fluid.
3. **Channel walls** (intensity ~15--25): Dark borders at the top and
   bottom of the channel, typically 5--15 pixels wide.
4. **Particles** (intensity ~150--255): Bright circular spots of 3--15
   pixel diameter, randomly distributed within the channel. These
   represent the fluorescent beads or cells being imaged.
5. **Sensor noise** (Gaussian, sigma ~2--5): Random per-pixel noise
   simulating sensor read noise.

Layout for a 640x480 image:

```
+----------------------------------------------+
|  Dark background (intensity ~20)             |  rows 0--120
|----------------------------------------------|
|  Channel wall (dark, 10px)                   |  rows 120--130
|  Channel interior (intensity ~60)            |  rows 130--350
|    * bright particles scattered inside       |
|  Channel wall (dark, 10px)                   |  rows 350--360
|----------------------------------------------|
|  Dark background (intensity ~20)             |  rows 360--480
+----------------------------------------------+
```

### 13.2 Exposure / Gain Response

The mock should simulate how exposure and gain affect image brightness:

```
effective_brightness = base_intensity * (exposure_ms / kRefExposure)
                       * gain_multiplier
```

Where `kRefExposure` = 10.0 ms is the reference exposure for nominal
brightness.

| Component | Base Intensity | After 20 ms, gain 2.0x |
|-----------|---------------|------------------------|
| Background | 20 | 80 |
| Channel | 60 | 240 |
| Particles | 200 | clamped to 255 |
| Noise sigma | 3 | 12 |

Clamp all values to [0, 255] after scaling.

**Overexposure simulation:** When brightness exceeds ~200 for the channel
region, particles begin to "bloom" (their diameter increases by 1--2 px)
and the image appears washed out.

### 13.3 Particle Simulation

Particles should be generated procedurally with deterministic seeding for
reproducibility:

- **Count:** 10--50 particles per frame (configurable)
- **Position:** Random within the channel region, uniform distribution
- **Diameter:** 5--12 pixels, Gaussian distribution (mean 8, sigma 2)
- **Intensity:** 150--255 relative to channel background
- **Shape:** Gaussian blob: `I(r) = peak * exp(-r^2 / (2 * sigma^2))`
  where sigma = diameter / 4
- **Motion:** For continuous capture, particles drift rightward at
  1--3 px/frame to simulate flow

For batch capture, each frame in the sequence should show particles at
slightly different positions to simulate real time-lapse behavior.

### 13.4 Implementation Sketch

```cpp
QImage MockCameraController::generateMicroscopyFrame() const {
  const int w = roi_.width();
  const int h = roi_.height();
  QImage frame(w, h, QImage::Format_Grayscale8);

  // Brightness scaling
  double scale = (exposure_ / kRefExposure) * gain_;

  // 1. Fill background
  uint8_t bg = clampByte(kBaseBackground * scale);
  frame.fill(bg);

  // 2. Draw channel region
  int ch_top = h * 0.25;
  int ch_bot = h * 0.75;
  int wall = 10;
  uint8_t ch_intensity = clampByte(kBaseChannel * scale);
  for (int y = ch_top + wall; y < ch_bot - wall; ++y) {
    memset(frame.scanLine(y), ch_intensity, w);
  }

  // 3. Draw channel walls (slightly darker than background)
  uint8_t wall_intensity = clampByte(kBaseWall * scale);
  for (int y = ch_top; y < ch_top + wall; ++y)
    memset(frame.scanLine(y), wall_intensity, w);
  for (int y = ch_bot - wall; y < ch_bot; ++y)
    memset(frame.scanLine(y), wall_intensity, w);

  // 4. Draw particles as Gaussian blobs
  std::mt19937 rng(frame_counter_);  // Deterministic per frame
  for (int i = 0; i < kParticleCount; ++i) {
    int px = std::uniform_int_distribution<>(0, w - 1)(rng);
    int py = std::uniform_int_distribution<>(
        ch_top + wall + 5, ch_bot - wall - 5)(rng);
    int diameter = std::normal_distribution<>(8.0, 2.0)(rng);
    double peak = std::uniform_real_distribution<>(150, 255)(rng);
    drawGaussianBlob(frame, px, py, diameter, peak * scale);
  }

  // 5. Add Gaussian noise
  double noise_sigma = kBaseNoiseSigma * scale;
  addGaussianNoise(frame, noise_sigma, rng);

  return frame;
}
```

---

## 14. References

1. [Basler Pylon C++ Programmer's Guide](https://docs.baslerweb.com/pylonapi/cpp/pylon_programmingguide) --
   Official SDK documentation
2. [Basler Pylon API Reference: CInstantCamera](https://docs.baslerweb.com/pylonapi/cpp/class_pylon_1_1_c_instant_camera) --
   Primary camera class
3. [Basler Pylon API Reference: CBaslerUniversalInstantCamera](https://docs.baslerweb.com/pylonapi/cpp/class_pylon_1_1_c_basler_universal_instant_camera) --
   Typed parameter access
4. [Basler Pylon API Reference: CGrabResultData](https://docs.baslerweb.com/pylonapi/cpp/class_pylon_1_1_c_grab_result_data) --
   Grab result methods
5. [Basler Pylon API Reference: CImageFormatConverter](https://docs.baslerweb.com/pylonapi/cpp/class_pylon_1_1_c_image_format_converter) --
   Pixel format conversion
6. [Basler Pylon API Reference: CImageEventHandler](https://docs.baslerweb.com/pylonapi/cpp/class_pylon_1_1_c_image_event_handler) --
   Asynchronous grab callback
7. [Basler Exposure Time Documentation](https://docs.baslerweb.com/exposure-time) --
   ExposureTime parameter details
8. [Basler Gain Documentation](https://docs.baslerweb.com/gain) --
   Gain parameter details
9. [Basler Pixel Format Documentation](https://docs.baslerweb.com/pixel-format) --
   Supported pixel formats
10. [Basler Pylon CMake on macOS](https://docs.baslerweb.com/knowledge/how-to-build-a-pylonbased-application-with-cmake-on-macos) --
    CMake build configuration
11. [Basler FindPylon.cmake (pylon-ros-camera)](https://github.com/basler/pylon-ros-camera/blob/master/pylon_camera/cmake/FindPylon.cmake) --
    Reference FindPylon module
12. [Basler acA2440-75um Product Page](https://www.baslerweb.com/en-us/shop/aca2440-75um/) --
    USB3 microscopy camera
13. [Basler acA1920-40gm Product Page](https://www.baslerweb.com/en-us/shop/aca1920-40gm/) --
    GigE microscopy camera
14. [GenICam SFNC 2.0 Specification](https://www.emva.org/wp-content/uploads/GenICam_SFNC_2_0_0.pdf) --
    Standard Feature Naming Convention

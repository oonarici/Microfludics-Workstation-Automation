/**
 * @file device_interface.cpp
 * @brief MOC compilation unit for the DeviceInterface abstract class.
 * @author MWA Team
 * @date 2026-03-22
 *
 * This file exists to provide a single translation unit for the
 * MOC-generated code of DeviceInterface, preventing duplicate symbol
 * errors when multiple libraries include device_interface.h.
 *
 * @copyright LGPL-3.0-or-later
 */

#include "device_interface.h"

namespace mwa::hardware {

DeviceInterface::~DeviceInterface() = default;

}  // namespace mwa::hardware

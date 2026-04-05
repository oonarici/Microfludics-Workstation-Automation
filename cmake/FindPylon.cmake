# FindPylon.cmake -- Locate the Basler Pylon C++ SDK
#
# This module finds the Basler Pylon SDK libraries and headers needed
# to build the real BaslerCameraController driver.  The SDK provides a
# C++ API for controlling Basler industrial cameras (ace, dart, pulse
# series) over GigE, USB3, or CoaXPress transports.
#
# Sets:
#   Pylon_FOUND         - TRUE if SDK found
#   Pylon_INCLUDE_DIRS  - Header search paths
#   Pylon_LIBRARIES     - Libraries to link
#
# Imported target:
#   Pylon::Pylon        - UNKNOWN IMPORTED library (when found)
#
# Hints:
#   PYLON_ROOT environment variable or CMake variable
#
# Platform notes:
#   - Windows: SDK installs to C:/Program Files/Basler/pylon X
#   - Linux:   SDK installs to /opt/pylon (or custom prefix)
#   - macOS:   SDK available since Pylon 6.x (limited model support)
#
# See also: docs/protocols/basler_pylon_spec.md

# ── Search paths ───────────────────────────────────────────────
set(_pylon_search_paths
  "$ENV{PYLON_ROOT}"
  "${PYLON_ROOT}"
  "C:/Program Files/Basler/pylon 7"
  "C:/Program Files/Basler/pylon 6"
  "C:/Program Files/Basler/pylon 5"
  "C:/Program Files (x86)/Basler/pylon 7"
  "C:/Program Files (x86)/Basler/pylon 6"
  "/opt/pylon"
  "/opt/pylon7"
  "/opt/pylon6"
  "$ENV{HOME}/pylon"
  "/Library/Frameworks"
)

# ── Find header ────────────────────────────────────────────────
find_path(Pylon_INCLUDE_DIR
  NAMES pylon/PylonIncludes.h
  PATHS ${_pylon_search_paths}
  PATH_SUFFIXES include
)

# ── Find library ───────────────────────────────────────────────
# Library name varies by platform:
#   - Linux:   libpylonbase.so
#   - Windows: PylonBase_v7_x.lib (version-specific) or pylonbase
#   - macOS:   libpylonbase.dylib
find_library(Pylon_LIBRARY
  NAMES pylonbase PylonBase_v7_4 PylonBase_v7_3 PylonBase_v7_2
        PylonBase_v7_1 PylonBase_v7_0 PylonBase_v6_3 PylonBase_v6_2
  PATHS ${_pylon_search_paths}
  PATH_SUFFIXES lib lib64 lib/x64
)

# ── Standard find-package handling ─────────────────────────────
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Pylon
  DEFAULT_MSG
  Pylon_INCLUDE_DIR
  Pylon_LIBRARY
)

# ── Create imported target ─────────────────────────────────────
if(Pylon_FOUND)
  set(Pylon_INCLUDE_DIRS "${Pylon_INCLUDE_DIR}")
  set(Pylon_LIBRARIES "${Pylon_LIBRARY}")

  if(NOT TARGET Pylon::Pylon)
    add_library(Pylon::Pylon UNKNOWN IMPORTED)
    set_target_properties(Pylon::Pylon PROPERTIES
      IMPORTED_LOCATION "${Pylon_LIBRARY}"
      INTERFACE_INCLUDE_DIRECTORIES "${Pylon_INCLUDE_DIR}"
    )
  endif()
endif()

mark_as_advanced(Pylon_INCLUDE_DIR Pylon_LIBRARY)

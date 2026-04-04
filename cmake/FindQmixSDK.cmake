# FindQmixSDK.cmake -- Locate the Cetoni QmixSDK C API
#
# This module finds the Cetoni QmixSDK libraries and headers needed to
# build the real PumpController driver.  The SDK provides a C API for
# controlling Nemesys syringe pumps over a CAN bus.
#
# Sets:
#   QmixSDK_FOUND         - TRUE if SDK found
#   QmixSDK_INCLUDE_DIRS  - Header search paths
#   QmixSDK_LIBRARIES     - Libraries to link
#
# Imported target:
#   QmixSDK::QmixSDK      - UNKNOWN IMPORTED library (when found)
#
# Hints:
#   QMIXSDK_ROOT environment variable or CMake variable
#
# Platform notes:
#   - Windows: Full SDK support (primary development platform)
#   - Linux:   SDK available since ~2022
#   - macOS:   SDK not available — this module sets QmixSDK_FOUND=FALSE
#
# See also: docs/protocols/cetoni_nemesys_spec.md

# ── macOS early-out ────────────────────────────────────────────
if(APPLE)
  set(QmixSDK_FOUND FALSE)
  if(NOT QmixSDK_FIND_QUIETLY)
    message(STATUS "FindQmixSDK: QmixSDK is not available on macOS")
  endif()
  return()
endif()

# ── Search paths ───────────────────────────────────────────────
set(_qmix_search_paths
  "$ENV{QMIXSDK_ROOT}"
  "${QMIXSDK_ROOT}"
  "C:/QmixSDK"
  "C:/Program Files/CETONI/QmixSDK"
  "C:/Program Files (x86)/CETONI/QmixSDK"
  "/opt/cetoni/qmixsdk"
  "$ENV{HOME}/QmixSDK"
)

# ── Find header ────────────────────────────────────────────────
find_path(QmixSDK_INCLUDE_DIR
  NAMES lcp.h
  PATHS ${_qmix_search_paths}
  PATH_SUFFIXES include lib/qmixsdk
)

# ── Find library ───────────────────────────────────────────────
find_library(QmixSDK_LIBRARY
  NAMES qmixsdk lcp
  PATHS ${_qmix_search_paths}
  PATH_SUFFIXES lib lib64
)

# ── Standard find-package handling ─────────────────────────────
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(QmixSDK
  DEFAULT_MSG
  QmixSDK_INCLUDE_DIR
  QmixSDK_LIBRARY
)

# ── Create imported target ─────────────────────────────────────
if(QmixSDK_FOUND)
  set(QmixSDK_INCLUDE_DIRS "${QmixSDK_INCLUDE_DIR}")
  set(QmixSDK_LIBRARIES "${QmixSDK_LIBRARY}")

  if(NOT TARGET QmixSDK::QmixSDK)
    add_library(QmixSDK::QmixSDK UNKNOWN IMPORTED)
    set_target_properties(QmixSDK::QmixSDK PROPERTIES
      IMPORTED_LOCATION "${QmixSDK_LIBRARY}"
      INTERFACE_INCLUDE_DIRECTORIES "${QmixSDK_INCLUDE_DIR}"
    )
  endif()
endif()

mark_as_advanced(QmixSDK_INCLUDE_DIR QmixSDK_LIBRARY)

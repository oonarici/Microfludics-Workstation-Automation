/**
 * @file panel_colors.h
 * @brief Shared colour constants for all device panel widgets.
 * @author MWA Team
 * @date 2026-03-24
 *
 * @copyright LGPL-3.0-or-later
 */

#pragma once

namespace mwa::gui {

/// @name Device Panel Status Colours
/// @{
/// Green — connected / idle.
inline constexpr auto kColorConnected    = "#27AE60";
/// Gray — disconnected.
inline constexpr auto kColorDisconnected = "#95A5A6";
/// Red — error state.
inline constexpr auto kColorError        = "#E74C3C";
/// Orange — connecting in progress.
inline constexpr auto kColorConnecting   = "#F39C12";
/// Blue — active operation (infusing, capturing, moving, etc.).
inline constexpr auto kColorActive       = "#2980B9";
/// @}

}  // namespace mwa::gui

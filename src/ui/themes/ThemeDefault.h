#pragma once
#include <cstdint>

namespace Theme {
    // Backgrounds
    constexpr uint32_t BG             = 0x000000;

    // Text
    constexpr uint32_t TEXT_PRIMARY   = 0xFFFFFF;
    constexpr uint32_t TEXT_DIM       = 0xAAAAAA;
    constexpr uint32_t TEXT_FAINT     = 0x888888;
    constexpr uint32_t TEXT_MUTED     = 0x666666;

    // Surfaces (arcs, tracks)
    constexpr uint32_t SURFACE        = 0x333333;
    constexpr uint32_t SURFACE_DARK   = 0x222222;
    constexpr uint32_t SURFACE_FAINT  = 0x444444;

    // Timer ring
    constexpr uint32_t RING_ACTIVE    = 0x666666;
    constexpr uint32_t RING_BG        = 0x1E1E1E;

    // Temperature scale
    constexpr uint32_t TEMP_COLD      = 0x0088FF;
    constexpr uint32_t TEMP_MID       = 0xFFCC00;
    constexpr uint32_t TEMP_WARM      = 0xFF6600;

    // AC modes
    constexpr uint32_t AC_MODE_OFF    = 0x333333;
    constexpr uint32_t AC_MODE_HEAT   = 0xFF4400;
    constexpr uint32_t AC_MODE_COOL   = 0x0088FF;
    constexpr uint32_t AC_MODE_AUTO   = 0x00BB44;
    constexpr uint32_t AC_MODE_FAN    = 0xBBBBBB;
    constexpr uint32_t AC_MODE_DRY    = 0xFFAA00;

    // Accents
    constexpr uint32_t ACCENT_RAIN    = 0x5599CC;
    constexpr uint32_t ACCENT_ERROR   = 0xFF3333;
}

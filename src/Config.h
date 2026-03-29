#pragma once

// How long a connection error must persist before the overlay appears.
// Prevents flashing the error screen during normal boot/reconnect.
static constexpr uint32_t WIFI_ERROR_DELAY_MS  = 10000; // 10s
static constexpr uint32_t HA_WS_ERROR_DELAY_MS = 10000; // 10s

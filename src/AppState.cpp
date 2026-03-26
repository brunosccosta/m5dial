#include "AppState.h"

AppState appState = {
    .lamps = {
        {"Living Room", false, 0},
        {"Bedroom",     true,  200},
        {"Kitchen",     false, 0},
        {"Office",      true,  128},
    },
    .lampCount = 4,

    .acs = {
        {"Living Room AC", 22.5f, 24.0f, "cool"},
        {"Bedroom AC",     23.0f, 22.0f, "heat"},
    },
    .acCount = 2,

    .room_temp = 22.5f,
    .dirty     = false,
};

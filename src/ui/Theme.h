#pragma once

#define THEME_DEFAULT 1

#ifndef ACTIVE_THEME
#define ACTIVE_THEME THEME_DEFAULT
#endif

#if ACTIVE_THEME == THEME_DEFAULT
#include "themes/ThemeDefault.h"
#endif

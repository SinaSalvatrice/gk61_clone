#ifndef LAYOUT
#    define LAYOUT LAYOUT_5x14
#endif
#pragma once

#include "quantum.h"

// Matrix pinout and diode direction for QMK
#define MATRIX_ROWS 5
#define MATRIX_COLS 14
#define MATRIX_ROW_PINS { GP0, GP1, GP2, GP3, GP4 }
#define MATRIX_COL_PINS { GP5, GP6, GP7, GP8, GP9, GP10, GP11, GP12, GP13, GP14, GP15, GP26, GP27, GP28 }
#define DIODE_DIRECTION COL2ROW

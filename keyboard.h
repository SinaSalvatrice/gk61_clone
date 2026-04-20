#pragma once

#include "quantum.h"

// Matrix pinout and diode direction for QMK
#define MATRIX_ROWS 5
#define MATRIX_COLS 14
#define MATRIX_ROW_PINS { GP0, GP1, GP2, GP3, GP4 }
#define MATRIX_COL_PINS { GP5, GP6, GP7, GP8, GP9, GP10, GP11, GP12, GP13, GP14, GP15, GP26, GP27, GP28 }
#define DIODE_DIRECTION COL2ROW

// True 5x14 matrix macro for full matrix keymap support
#define LAYOUT_5x14( \
	K00, K01, K02, K03, K04, K05, K06, K07, K08, K09, K0A, K0B, K0C, K0D, \
	K10, K11, K12, K13, K14, K15, K16, K17, K18, K19, K1A, K1B, K1C, K1D, \
	K20, K21, K22, K23, K24, K25, K26, K27, K28, K29, K2A, K2B, K2C, K2D, \
	K30, K31, K32, K33, K34, K35, K36, K37, K38, K39, K3A, K3B, K3C, K3D, \
	K40, K41, K42, K43, K44, K45, K46, K47, K48, K49, K4A, K4B, K4C, K4D  \
) \
{ \
	{ K00, K01, K02, K03, K04, K05, K06, K07, K08, K09, K0A, K0B, K0C, K0D }, \
	{ K10, K11, K12, K13, K14, K15, K16, K17, K18, K19, K1A, K1B, K1C, K1D }, \
	{ K20, K21, K22, K23, K24, K25, K26, K27, K28, K29, K2A, K2B, K2C, K2D }, \
	{ K30, K31, K32, K33, K34, K35, K36, K37, K38, K39, K3A, K3B, K3C, K3D }, \
	{ K40, K41, K42, K43, K44, K45, K46, K47, K48, K49, K4A, K4B, K4C, K4D }  \
}


#ifndef CONFIG_H
#define CONFIG_H

// Matrix pinout to match arduino_matrix_diag.ino
#define MATRIX_ROWS 5
#define MATRIX_COLS 14
#define MATRIX_ROW_PINS { GP0, GP1, GP2, GP3, GP4 }
#define MATRIX_COL_PINS { GP5, GP6, GP7, GP8, GP9, GP10, GP11, GP12, GP13, GP14, GP15, GP26, GP27, GP28 }
#define DIODE_DIRECTION COL2ROW

// VIA requires a unique keyboard UID for detection
#define VIA_KEYBOARD_UID { 0xA1, 0xB2, 0xC3, 0xD4, 0xE5, 0xF6, 0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF, 0x10, 0x20 }

#endif // CONFIG_H

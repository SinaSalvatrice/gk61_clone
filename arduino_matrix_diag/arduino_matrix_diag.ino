#include <Arduino.h>
#if __has_include(<Keyboard.h>)
  #include <Keyboard.h>
  #define ENABLE_USB_KEYBOARD 1
#else
  #define ENABLE_USB_KEYBOARD 0
#endif

#if __has_include(<Adafruit_NeoPixel.h>)
  #include <Adafruit_NeoPixel.h>
  #define ENABLE_RGB 1
#else
  #define ENABLE_RGB 0
#endif

#include <math.h>

// ============================================================
// RP2040 61-key keyboard sketch
// ------------------------------------------------------------
// - 5x14 physical matrix
// - logical 61-key layout with gaps / big keys
// - USB keyboard output
// - momentary FN layer
// - RGB layer effects
// - REMAP layer between raw matrix and logical keymap
//
// IMPORTANT:
// The scan stays physical.
// The keymap stays logical.
// If one row shifts after Y, fix ONLY the remap table.
// ============================================================

// ---------------------------
// Pins
// ---------------------------
constexpr uint8_t ROW_PINS[] = {0, 1, 2, 3, 4};
constexpr uint8_t COL_PINS[] = {5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 26, 27, 28};

constexpr size_t ROW_COUNT = sizeof(ROW_PINS) / sizeof(ROW_PINS[0]);
constexpr size_t COL_COUNT = sizeof(COL_PINS) / sizeof(COL_PINS[0]);

// ---------------------------
// RGB
// ---------------------------
constexpr uint8_t RGB_PIN = 29;
constexpr uint16_t RGB_LED_COUNT = 36;
constexpr uint8_t RGB_BRIGHTNESS = 36;

// ---------------------------
// Timing
// ---------------------------
constexpr uint16_t SCAN_SETTLE_US   = 30;
constexpr uint16_t LOOP_DELAY_MS    = 5;
constexpr uint16_t RGB_FRAME_MS     = 24;
constexpr uint16_t REACTIVE_MS      = 90;
constexpr uint16_t IDLE_PRINT_MS    = 2500;
constexpr uint16_t HEARTBEAT_MS     = 500;

// ---------------------------
// FN key in LOGICAL matrix
// keep from your original sketch
// ---------------------------
constexpr size_t FN_ROW = 4;
constexpr size_t FN_COL = 10;

// ============================================================
// HID helpers
// ============================================================
#define HID_KEY(usage) static_cast<uint8_t>((usage) + 136)

#ifndef KEY_LEFT_CTRL
  #define KEY_LEFT_CTRL 128
#endif
#ifndef KEY_LEFT_SHIFT
  #define KEY_LEFT_SHIFT 129
#endif
#ifndef KEY_LEFT_ALT
  #define KEY_LEFT_ALT 130
#endif
#ifndef KEY_LEFT_GUI
  #define KEY_LEFT_GUI 131
#endif
#ifndef KEY_RIGHT_CTRL
  #define KEY_RIGHT_CTRL 132
#endif
#ifndef KEY_RIGHT_SHIFT
  #define KEY_RIGHT_SHIFT 133
#endif
#ifndef KEY_RIGHT_ALT
  #define KEY_RIGHT_ALT 134
#endif
#ifndef KEY_RIGHT_GUI
  #define KEY_RIGHT_GUI 135
#endif

#ifndef KEY_MENU
  #define KEY_MENU HID_KEY(0x65)
#endif
#ifndef KEY_ESC
  #define KEY_ESC HID_KEY(0x29)
#endif
#ifndef KEY_BACKSPACE
  #define KEY_BACKSPACE HID_KEY(0x2A)
#endif
#ifndef KEY_TAB
  #define KEY_TAB HID_KEY(0x2B)
#endif
#ifndef KEY_RETURN
  #define KEY_RETURN HID_KEY(0x28)
#endif
#ifndef KEY_CAPS_LOCK
  #define KEY_CAPS_LOCK HID_KEY(0x39)
#endif
#ifndef KEY_DELETE
  #define KEY_DELETE HID_KEY(0x4C)
#endif
#ifndef KEY_UP_ARROW
  #define KEY_UP_ARROW HID_KEY(0x52)
#endif
#ifndef KEY_DOWN_ARROW
  #define KEY_DOWN_ARROW HID_KEY(0x51)
#endif
#ifndef KEY_LEFT_ARROW
  #define KEY_LEFT_ARROW HID_KEY(0x50)
#endif
#ifndef KEY_RIGHT_ARROW
  #define KEY_RIGHT_ARROW HID_KEY(0x4F)
#endif
#ifndef KEY_INSERT
  #define KEY_INSERT HID_KEY(0x49)
#endif
#ifndef KEY_HOME
  #define KEY_HOME HID_KEY(0x4A)
#endif
#ifndef KEY_PAGE_UP
  #define KEY_PAGE_UP HID_KEY(0x4B)
#endif
#ifndef KEY_END
  #define KEY_END HID_KEY(0x4D)
#endif
#ifndef KEY_PAGE_DOWN
  #define KEY_PAGE_DOWN HID_KEY(0x4E)
#endif
#ifndef KEY_PRINT_SCREEN
  #define KEY_PRINT_SCREEN HID_KEY(0x46)
#endif
#ifndef KEY_SCROLL_LOCK
  #define KEY_SCROLL_LOCK HID_KEY(0x47)
#endif
#ifndef KEY_PAUSE
  #define KEY_PAUSE HID_KEY(0x48)
#endif
#ifndef KEY_F1
  #define KEY_F1 HID_KEY(0x3A)
#endif
#ifndef KEY_F2
  #define KEY_F2 HID_KEY(0x3B)
#endif
#ifndef KEY_F3
  #define KEY_F3 HID_KEY(0x3C)
#endif
#ifndef KEY_F4
  #define KEY_F4 HID_KEY(0x3D)
#endif
#ifndef KEY_F5
  #define KEY_F5 HID_KEY(0x3E)
#endif
#ifndef KEY_F6
  #define KEY_F6 HID_KEY(0x3F)
#endif
#ifndef KEY_F7
  #define KEY_F7 HID_KEY(0x40)
#endif
#ifndef KEY_F8
  #define KEY_F8 HID_KEY(0x41)
#endif
#ifndef KEY_F9
  #define KEY_F9 HID_KEY(0x42)
#endif
#ifndef KEY_F10
  #define KEY_F10 HID_KEY(0x43)
#endif
#ifndef KEY_F11
  #define KEY_F11 HID_KEY(0x44)
#endif
#ifndef KEY_F12
  #define KEY_F12 HID_KEY(0x45)
#endif

// ============================================================
// Key definitions
// ============================================================
enum KeyKind : uint8_t {
  KEY_KIND_NONE,
  KEY_KIND_CODE,
  KEY_KIND_FN,
  KEY_KIND_CTRL_L
};

struct KeyDef {
  KeyKind kind;
  uint8_t code;
};

#define NO_KEY      {KEY_KIND_NONE, 0}
#define CODE_KEY(c) {KEY_KIND_CODE, static_cast<uint8_t>(c)}
#define FN_KEY      {KEY_KIND_FN, 0}
#define CTRL_L_KEY  {KEY_KIND_CTRL_L, 0}

// ============================================================
// REMAP layer
// physical raw matrix -> logical matrix
//
// If one row shifts after Y, FIX THIS TABLE, not scan code.
//
// Format:
// logical[row][col] = physical[row][col]
//
// Use {-1,-1} for logical gaps.
// ============================================================
constexpr int8_t NO_POS = -1;

struct MatrixPos {
  int8_t row;
  int8_t col;
};

// -----------------------------------------------------------------
// DEFAULT REMAP
// -----------------------------------------------------------------
// Right now this is mostly 1:1, BUT row 1 has an optional "gap after Y"
// because that is exactly the failure you described.
//
// If that guess is wrong, adjust ONLY row 1 below.
//
// Logical row 1 currently behaves as:
// TAB Q W E R T Y [gap] U I O P [ ] \
//
// So if your raw matrix actually reports Y,U,I,O,P contiguous,
// but your logical map needs one empty slot after Y,
// this fixes it.
//
// If it becomes worse, remove the gap and make row 1 contiguous again.
// -----------------------------------------------------------------
const MatrixPos LOGICAL_TO_PHYSICAL[ROW_COUNT][COL_COUNT] = {
  // Row 0
  {
    {0,0}, {0,1}, {0,2}, {0,3}, {0,4}, {0,5}, {0,6},
    {0,7}, {0,8}, {0,9}, {0,10}, {0,11}, {0,12}, {0,13}
  },

  // Row 1  <-- likely problem row around Y/U
  {
    {1,0},  // TAB
    {1,1},  // Q
    {1,2},  // W
    {1,3},  // E
    {1,4},  // R
    {1,5},  // T
    {1,6},  // Y

    {NO_POS, NO_POS}, // gap after Y -> adjust here if needed

    {1,7},  // U
    {1,8},  // I
    {1,9},  // O
    {1,10}, // P
    {1,11}, // [
    {1,12}  // ]
    // NOTE:
    // If you need backslash too, move row content accordingly.
    // This row is the one to tune.
  },

  // Row 2
  {
    {2,0}, {2,1}, {2,2}, {2,3}, {2,4}, {2,5}, {2,6},
    {2,7}, {2,8}, {2,9}, {2,10}, {2,11}, {2,12}, {NO_POS, NO_POS}
  },

  // Row 3
  {
    {3,0}, {3,1}, {3,2}, {3,3}, {3,4}, {3,5}, {3,6},
    {3,7}, {3,8}, {3,9}, {3,10}, {NO_POS, NO_POS}, {NO_POS, NO_POS}, {3,13}
  },

  // Row 4
  {
    {4,0}, {4,1}, {4,2}, {NO_POS, NO_POS}, {NO_POS, NO_POS}, {4,5},
    {NO_POS, NO_POS}, {NO_POS, NO_POS}, {NO_POS, NO_POS}, {4,9}, {4,10}, {4,11}, {4,12}, {4,13}
  }
};

// ============================================================
// Logical keymaps
// These stay "human readable".
// The remap above decides where the raw key really comes from.
// ============================================================

const KeyDef BASE_LAYER[ROW_COUNT][COL_COUNT] = {
  {
    CODE_KEY(KEY_ESC),
    CODE_KEY(HID_KEY(0x1E)), CODE_KEY(HID_KEY(0x1F)), CODE_KEY(HID_KEY(0x20)),
    CODE_KEY(HID_KEY(0x21)), CODE_KEY(HID_KEY(0x22)), CODE_KEY(HID_KEY(0x23)),
    CODE_KEY(HID_KEY(0x24)), CODE_KEY(HID_KEY(0x25)), CODE_KEY(HID_KEY(0x26)),
    CODE_KEY(HID_KEY(0x27)), CODE_KEY(HID_KEY(0x2D)), CODE_KEY(HID_KEY(0x2E)),
    CODE_KEY(KEY_BACKSPACE)
  },
  {
    CODE_KEY(KEY_TAB),
    CODE_KEY(HID_KEY(0x14)), CODE_KEY(HID_KEY(0x1A)), CODE_KEY(HID_KEY(0x08)),
    CODE_KEY(HID_KEY(0x15)), CODE_KEY(HID_KEY(0x17)), CODE_KEY(HID_KEY(0x1C)),
    CODE_KEY(HID_KEY(0x18)), CODE_KEY(HID_KEY(0x0C)), CODE_KEY(HID_KEY(0x12)),
    CODE_KEY(HID_KEY(0x13)), CODE_KEY(HID_KEY(0x2F)), CODE_KEY(HID_KEY(0x30)),
    CODE_KEY(HID_KEY(0x31))
  },
  {
    CODE_KEY(KEY_CAPS_LOCK),
    CODE_KEY(HID_KEY(0x04)), CODE_KEY(HID_KEY(0x16)), CODE_KEY(HID_KEY(0x07)),
    CODE_KEY(HID_KEY(0x09)), CODE_KEY(HID_KEY(0x0A)), CODE_KEY(HID_KEY(0x0B)),
    CODE_KEY(HID_KEY(0x0D)), CODE_KEY(HID_KEY(0x0E)), CODE_KEY(HID_KEY(0x0F)),
    CODE_KEY(HID_KEY(0x33)), CODE_KEY(HID_KEY(0x34)), CODE_KEY(KEY_RETURN),
    NO_KEY
  },
  {
    CODE_KEY(KEY_LEFT_SHIFT),
    CODE_KEY(HID_KEY(0x1D)), CODE_KEY(HID_KEY(0x1B)), CODE_KEY(HID_KEY(0x06)),
    CODE_KEY(HID_KEY(0x19)), CODE_KEY(HID_KEY(0x05)), CODE_KEY(HID_KEY(0x11)),
    CODE_KEY(HID_KEY(0x10)), CODE_KEY(HID_KEY(0x36)), CODE_KEY(HID_KEY(0x37)),
    CODE_KEY(HID_KEY(0x38)), NO_KEY, NO_KEY, CODE_KEY(KEY_RIGHT_SHIFT)
  },
  {
    CODE_KEY(KEY_LEFT_CTRL), CODE_KEY(KEY_LEFT_GUI), CODE_KEY(KEY_LEFT_ALT),
    NO_KEY, NO_KEY,
    CODE_KEY(HID_KEY(0x2C)), // space
    NO_KEY, NO_KEY, NO_KEY,
    CODE_KEY(KEY_RIGHT_ALT),
    FN_KEY,
    CODE_KEY(KEY_MENU),
    CTRL_L_KEY,
    CODE_KEY(KEY_RIGHT_CTRL)
  }
};

const KeyDef FN_LAYER[ROW_COUNT][COL_COUNT] = {
  {
    CODE_KEY(KEY_ESC),
    CODE_KEY(KEY_F1), CODE_KEY(KEY_F2), CODE_KEY(KEY_F3), CODE_KEY(KEY_F4),
    CODE_KEY(KEY_F5), CODE_KEY(KEY_F6), CODE_KEY(KEY_F7), CODE_KEY(KEY_F8),
    CODE_KEY(KEY_F9), CODE_KEY(KEY_F10), CODE_KEY(KEY_F11), CODE_KEY(KEY_F12),
    CODE_KEY(KEY_DELETE)
  },
  {
    NO_KEY,
    NO_KEY, CODE_KEY(KEY_UP_ARROW), NO_KEY, NO_KEY, NO_KEY, NO_KEY,
    NO_KEY, NO_KEY, CODE_KEY(KEY_PRINT_SCREEN), CODE_KEY(KEY_SCROLL_LOCK),
    CODE_KEY(KEY_PAUSE), NO_KEY, NO_KEY
  },
  {
    NO_KEY,
    CODE_KEY(KEY_LEFT_ARROW), CODE_KEY(KEY_DOWN_ARROW), CODE_KEY(KEY_RIGHT_ARROW),
    NO_KEY, NO_KEY, NO_KEY, NO_KEY, NO_KEY,
    CODE_KEY(KEY_INSERT), CODE_KEY(KEY_HOME), CODE_KEY(KEY_PAGE_UP),
    CODE_KEY(KEY_RETURN), NO_KEY
  },
  {
    NO_KEY,
    NO_KEY, NO_KEY, NO_KEY, NO_KEY, NO_KEY, NO_KEY, NO_KEY, NO_KEY,
    CODE_KEY(KEY_DELETE), CODE_KEY(KEY_END), NO_KEY, NO_KEY, CODE_KEY(KEY_PAGE_DOWN)
  },
  {
    NO_KEY, NO_KEY, NO_KEY, NO_KEY, NO_KEY, NO_KEY, NO_KEY,
    NO_KEY, NO_KEY, NO_KEY, FN_KEY, NO_KEY, NO_KEY, NO_KEY
  }
};

// ============================================================
// State
// ============================================================
bool rawMatrix[ROW_COUNT][COL_COUNT] = {};
bool logicalMatrix[ROW_COUNT][COL_COUNT] = {};
bool lastLogicalMatrix[ROW_COUNT][COL_COUNT] = {};

bool lastFnActive = false;
unsigned long lastIdleReport = 0;
unsigned long lastHeartbeat = 0;
bool ledState = false;

#if ENABLE_RGB
Adafruit_NeoPixel rgb(RGB_LED_COUNT, RGB_PIN, NEO_GRB + NEO_KHZ800);
unsigned long lastRgbUpdate = 0;
unsigned long reactiveUntil = 0;
uint16_t rgbTick = 0;
#endif

// ============================================================
// Helpers
// ============================================================
void setAllInputsPullup() {
  for (size_t r = 0; r < ROW_COUNT; ++r) {
    pinMode(ROW_PINS[r], INPUT_PULLUP);
  }
  for (size_t c = 0; c < COL_COUNT; ++c) {
    pinMode(COL_PINS[c], INPUT_PULLUP);
  }
}

void clearMatrix(bool m[ROW_COUNT][COL_COUNT]) {
  memset(m, 0, sizeof(bool) * ROW_COUNT * COL_COUNT);
}

void scanMatrix_COL2ROW(bool m[ROW_COUNT][COL_COUNT]) {
  clearMatrix(m);
  setAllInputsPullup();

  for (size_t row = 0; row < ROW_COUNT; ++row) {
    pinMode(ROW_PINS[row], OUTPUT);
    digitalWrite(ROW_PINS[row], LOW);
    delayMicroseconds(SCAN_SETTLE_US);

    for (size_t col = 0; col < COL_COUNT; ++col) {
      m[row][col] = (digitalRead(COL_PINS[col]) == LOW);
    }

    pinMode(ROW_PINS[row], INPUT_PULLUP);
  }
}

void buildLogicalMatrix(bool logical[ROW_COUNT][COL_COUNT],
                        const bool physical[ROW_COUNT][COL_COUNT]) {
  for (size_t row = 0; row < ROW_COUNT; ++row) {
    for (size_t col = 0; col < COL_COUNT; ++col) {
      logical[row][col] = false;

      const MatrixPos &p = LOGICAL_TO_PHYSICAL[row][col];
      if (p.row < 0 || p.col < 0) {
        continue;
      }

      logical[row][col] = physical[p.row][p.col];
    }
  }
}

bool matricesEqual(const bool a[ROW_COUNT][COL_COUNT],
                   const bool b[ROW_COUNT][COL_COUNT]) {
  return memcmp(a, b, sizeof(bool) * ROW_COUNT * COL_COUNT) == 0;
}

bool matrixHasPress(const bool m[ROW_COUNT][COL_COUNT]) {
  for (size_t r = 0; r < ROW_COUNT; ++r) {
    for (size_t c = 0; c < COL_COUNT; ++c) {
      if (m[r][c]) return true;
    }
  }
  return false;
}

void copyMatrix(bool dst[ROW_COUNT][COL_COUNT],
                const bool src[ROW_COUNT][COL_COUNT]) {
  memcpy(dst, src, sizeof(bool) * ROW_COUNT * COL_COUNT);
}

const KeyDef &getKeyDef(bool fnActive, size_t row, size_t col) {
  return fnActive ? FN_LAYER[row][col] : BASE_LAYER[row][col];
}

void pressKeyDef(const KeyDef &key) {
#if ENABLE_USB_KEYBOARD
  switch (key.kind) {
    case KEY_KIND_CODE:
      if (key.code != 0) Keyboard.press(key.code);
      break;
    case KEY_KIND_CTRL_L:
      Keyboard.press(KEY_LEFT_CTRL);
      Keyboard.press('l');
      break;
    default:
      break;
  }
#else
  (void)key;
#endif
}

void releaseKeyDef(const KeyDef &key) {
#if ENABLE_USB_KEYBOARD
  switch (key.kind) {
    case KEY_KIND_CODE:
      if (key.code != 0) Keyboard.release(key.code);
      break;
    case KEY_KIND_CTRL_L:
      Keyboard.release('l');
      Keyboard.release(KEY_LEFT_CTRL);
      break;
    default:
      break;
  }
#else
  (void)key;
#endif
}

void rebuildKeyboardReport(const bool current[ROW_COUNT][COL_COUNT], bool fnActive) {
#if ENABLE_USB_KEYBOARD
  Keyboard.releaseAll();

  for (size_t row = 0; row < ROW_COUNT; ++row) {
    for (size_t col = 0; col < COL_COUNT; ++col) {
      if (!current[row][col]) continue;
      if (row == FN_ROW && col == FN_COL) continue;
      pressKeyDef(getKeyDef(fnActive, row, col));
    }
  }
#else
  (void)current;
  (void)fnActive;
#endif
}

void printLogicalHits(const bool m[ROW_COUNT][COL_COUNT]) {
  Serial.println("LOGICAL hits:");
  bool any = false;

  for (size_t row = 0; row < ROW_COUNT; ++row) {
    for (size_t col = 0; col < COL_COUNT; ++col) {
      if (!m[row][col]) continue;
      any = true;
      Serial.print("  logical r");
      Serial.print(row);
      Serial.print(" c");
      Serial.print(col);

      const MatrixPos &p = LOGICAL_TO_PHYSICAL[row][col];
      Serial.print("  <- physical r");
      Serial.print(p.row);
      Serial.print(" c");
      Serial.println(p.col);
    }
  }

  if (!any) {
    Serial.println("  no keys detected");
  }
}

#if ENABLE_RGB
void fillRgb(uint8_t r, uint8_t g, uint8_t b) {
  for (uint16_t i = 0; i < RGB_LED_COUNT; ++i) {
    rgb.setPixelColor(i, rgb.Color(r, g, b));
  }
}

void triggerReactiveFlash() {
  reactiveUntil = millis() + REACTIVE_MS;
}

void drawBaseEffect() {
  // blue / purple flowing wave
  for (uint16_t i = 0; i < RGB_LED_COUNT; ++i) {
    float phase = (float)i * 0.33f + (float)rgbTick * 0.11f;
    float s = (sinf(phase) + 1.0f) * 0.5f; // 0..1

    uint8_t r = (uint8_t)(18 + 55 * s);
    uint8_t g = 0;
    uint8_t b = (uint8_t)(70 + 110 * s);

    rgb.setPixelColor(i, rgb.Color(r, g, b));
  }
}

void drawFnEffect() {
  // green / yellow breathing sweep
  for (uint16_t i = 0; i < RGB_LED_COUNT; ++i) {
    float phase = (float)rgbTick * 0.09f + (float)i * 0.15f;
    float s = (sinf(phase) + 1.0f) * 0.5f;

    uint8_t r = (uint8_t)(25 + 120 * s);
    uint8_t g = (uint8_t)(75 + 145 * s);
    uint8_t b = 0;

    rgb.setPixelColor(i, rgb.Color(r, g, b));
  }
}

void updateRgb(bool fnActive) {
  if (millis() - lastRgbUpdate < RGB_FRAME_MS) return;
  lastRgbUpdate = millis();

  if ((long)(reactiveUntil - millis()) > 0) {
    fillRgb(100, 100, 100);
  } else {
    if (fnActive) drawFnEffect();
    else drawBaseEffect();
  }

  rgb.show();
  ++rgbTick;
}
#endif

void updateKeyboardFromLogicalMatrix(const bool current[ROW_COUNT][COL_COUNT]) {
  const bool fnActive = current[FN_ROW][FN_COL];
  bool newPress = false;

  if (fnActive != lastFnActive) {
    rebuildKeyboardReport(current, fnActive);
    newPress = true;
  } else {
    for (size_t row = 0; row < ROW_COUNT; ++row) {
      for (size_t col = 0; col < COL_COUNT; ++col) {
        if (row == FN_ROW && col == FN_COL) continue;

        const bool now = current[row][col];
        const bool before = lastLogicalMatrix[row][col];

        if (now && !before) {
          pressKeyDef(getKeyDef(fnActive, row, col));
          newPress = true;
        } else if (!now && before) {
          releaseKeyDef(getKeyDef(lastFnActive, row, col));
        }
      }
    }
  }

#if ENABLE_RGB
  if (newPress) {
    triggerReactiveFlash();
  }
#endif

  copyMatrix(lastLogicalMatrix, current);
  lastFnActive = fnActive;
}

// ============================================================
// Setup / Loop
// ============================================================
void setup() {
#ifdef LED_BUILTIN
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
#endif

  setAllInputsPullup();

#if ENABLE_USB_KEYBOARD
  Keyboard.begin();
#endif

#if ENABLE_RGB
  rgb.begin();
  rgb.setBrightness(RGB_BRIGHTNESS);
  rgb.clear();
  rgb.show();
#endif

  Serial.begin(115200);
  unsigned long start = millis();
  while (!Serial && (millis() - start) < 4000) {}

  Serial.println();
  Serial.println("RP2040 61-key keyboard");
  Serial.println("Scan: COL2ROW");
  Serial.println("Mode: raw matrix -> remap -> logical keymap");
  Serial.println("If Y/U/I/O/P still shift: adjust LOGICAL_TO_PHYSICAL row 1 only.");
#if !ENABLE_USB_KEYBOARD
  Serial.println("WARNING: Keyboard.h not available in this core.");
#endif
#if !ENABLE_RGB
  Serial.println("WARNING: Adafruit_NeoPixel not found; RGB disabled.");
#endif
  Serial.println();
}

void loop() {
  scanMatrix_COL2ROW(rawMatrix);
  buildLogicalMatrix(logicalMatrix, rawMatrix);
  updateKeyboardFromLogicalMatrix(logicalMatrix);

  const bool changed = !matricesEqual(logicalMatrix, lastLogicalMatrix);
  const bool active = matrixHasPress(logicalMatrix);

  if (changed) {
    Serial.println("------------------------------");
    printLogicalHits(logicalMatrix);
    Serial.println();
  } else if (!active && millis() - lastIdleReport > IDLE_PRINT_MS) {
    Serial.println("idle: no logical keys detected");
    lastIdleReport = millis();
  }

#if ENABLE_RGB
  updateRgb(logicalMatrix[FN_ROW][FN_COL]);
#endif

#ifdef LED_BUILTIN
  if (millis() - lastHeartbeat > HEARTBEAT_MS) {
    ledState = !ledState;
    digitalWrite(LED_BUILTIN, ledState ? HIGH : LOW);
    lastHeartbeat = millis();
  }
#endif

  delay(LOOP_DELAY_MS);
}
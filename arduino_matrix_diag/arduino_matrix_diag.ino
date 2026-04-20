#include <Arduino.h>
#if __has_include(<Keyboard.h>)
#include <Keyboard.h>
#define ENABLE_USB_KEYBOARD 1
#else
#define ENABLE_USB_KEYBOARD 0
#endif

#define RGB_TEST_ENABLE 1

#if RGB_TEST_ENABLE && __has_include(<Adafruit_NeoPixel.h>)
#include <Adafruit_NeoPixel.h>
#define ENABLE_RGB_TEST 1
#else
#define ENABLE_RGB_TEST 0
#endif

constexpr uint8_t ROW_PINS[] = {0, 1, 2, 3, 4};
constexpr uint8_t COL_PINS[] = {5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 26, 27, 28};
#if RGB_TEST_ENABLE
constexpr uint8_t PROBE_PINS[] = {16, 17, 18, 19, 20, 21, 22, 23, 24, 25};
#else
constexpr uint8_t PROBE_PINS[] = {16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 29};
#endif

constexpr uint8_t RGB_TEST_PIN = 29;
constexpr uint16_t RGB_TEST_LED_COUNT = 36;

#define KEYBOARD_MODE 1
#define MATRIX_DIRECTION_COL2ROW 1

constexpr size_t ROW_COUNT = sizeof(ROW_PINS) / sizeof(ROW_PINS[0]);
constexpr size_t COL_COUNT = sizeof(COL_PINS) / sizeof(COL_PINS[0]);
constexpr size_t PROBE_COUNT = sizeof(PROBE_PINS) / sizeof(PROBE_PINS[0]);
constexpr size_t FULL_COL_COUNT = COL_COUNT;

bool last_row2col[ROW_COUNT][COL_COUNT] = {};
#if !KEYBOARD_MODE
bool last_col2row[ROW_COUNT][COL_COUNT] = {};
bool last_probe_row2col[ROW_COUNT][PROBE_COUNT] = {};
bool last_probe_col2row[ROW_COUNT][PROBE_COUNT] = {};
#endif


#if ENABLE_USB_KEYBOARD
enum KeyKind : uint8_t {
  KEY_KIND_NONE,
  KEY_KIND_CHAR,
  KEY_KIND_CODE,
  KEY_KIND_FN,
  KEY_KIND_CTRL_L, // Special: Ctrl+L
};

struct KeyDef {
  KeyKind kind;
  uint8_t code;
};

#define HID_KEY(usage) static_cast<uint8_t>((usage) + 136)
#define NO_KEY      {KEY_KIND_NONE, 0}
#define CHAR_KEY(c) {KEY_KIND_CHAR, static_cast<uint8_t>(c)}
#define CODE_KEY(c) {KEY_KIND_CODE, static_cast<uint8_t>(c)}
#define FN_KEY      {KEY_KIND_FN, 0}
#define CTRL_L_KEY  {KEY_KIND_CTRL_L, 0}

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

constexpr size_t FN_ROW = 4;
constexpr size_t FN_COL = 10;

const KeyDef BASE_LAYER[ROW_COUNT][FULL_COL_COUNT] = {
  {CODE_KEY(KEY_ESC), CODE_KEY(HID_KEY(0x1E)), CODE_KEY(HID_KEY(0x1F)), CODE_KEY(HID_KEY(0x20)), CODE_KEY(HID_KEY(0x21)), CODE_KEY(HID_KEY(0x22)), CODE_KEY(HID_KEY(0x23)), CODE_KEY(HID_KEY(0x24)), CODE_KEY(HID_KEY(0x25)), CODE_KEY(HID_KEY(0x26)), CODE_KEY(HID_KEY(0x27)), CODE_KEY(HID_KEY(0x2D)), CODE_KEY(HID_KEY(0x2E)), CODE_KEY(KEY_BACKSPACE)},
  {CODE_KEY(KEY_TAB), CODE_KEY(HID_KEY(0x14)), CODE_KEY(HID_KEY(0x1A)), CODE_KEY(HID_KEY(0x08)), CODE_KEY(HID_KEY(0x15)), CODE_KEY(HID_KEY(0x17)), CODE_KEY(HID_KEY(0x1C)), CODE_KEY(HID_KEY(0x18)), CODE_KEY(HID_KEY(0x0C)), CODE_KEY(HID_KEY(0x12)), CODE_KEY(HID_KEY(0x13)), CODE_KEY(HID_KEY(0x2F)), CODE_KEY(HID_KEY(0x30)), CODE_KEY(HID_KEY(0x31))},
  {CODE_KEY(KEY_CAPS_LOCK), CODE_KEY(HID_KEY(0x04)), CODE_KEY(HID_KEY(0x16)), CODE_KEY(HID_KEY(0x07)), CODE_KEY(HID_KEY(0x09)), CODE_KEY(HID_KEY(0x0A)), CODE_KEY(HID_KEY(0x0B)), CODE_KEY(HID_KEY(0x0D)), CODE_KEY(HID_KEY(0x0E)), CODE_KEY(HID_KEY(0x0F)), CODE_KEY(HID_KEY(0x33)), CODE_KEY(HID_KEY(0x34)), CODE_KEY(KEY_RETURN), NO_KEY},
  {CODE_KEY(KEY_LEFT_SHIFT), CODE_KEY(HID_KEY(0x1D)), CODE_KEY(HID_KEY(0x1B)), CODE_KEY(HID_KEY(0x06)), CODE_KEY(HID_KEY(0x19)), CODE_KEY(HID_KEY(0x05)), CODE_KEY(HID_KEY(0x11)), CODE_KEY(HID_KEY(0x10)), CODE_KEY(HID_KEY(0x36)), CODE_KEY(HID_KEY(0x37)), CODE_KEY(HID_KEY(0x38)), NO_KEY, NO_KEY, CODE_KEY(KEY_RIGHT_SHIFT)},
  {CODE_KEY(KEY_LEFT_CTRL), CODE_KEY(KEY_LEFT_GUI), CODE_KEY(KEY_LEFT_ALT), NO_KEY, NO_KEY, CODE_KEY(HID_KEY(0x2C)), NO_KEY, NO_KEY, NO_KEY, CODE_KEY(KEY_RIGHT_ALT), FN_KEY, CODE_KEY(KEY_MENU), CTRL_L_KEY, CODE_KEY(KEY_RIGHT_CTRL)},
};

const KeyDef FN_LAYER[ROW_COUNT][FULL_COL_COUNT] = {
  {CODE_KEY(KEY_ESC), CODE_KEY(KEY_F1), CODE_KEY(KEY_F2), CODE_KEY(KEY_F3), CODE_KEY(KEY_F4), CODE_KEY(KEY_F5), CODE_KEY(KEY_F6), CODE_KEY(KEY_F7), CODE_KEY(KEY_F8), CODE_KEY(KEY_F9), CODE_KEY(KEY_F10), CODE_KEY(KEY_F11), CODE_KEY(KEY_F12), CODE_KEY(KEY_DELETE)},
  {NO_KEY, NO_KEY, CODE_KEY(KEY_UP_ARROW), NO_KEY, NO_KEY, NO_KEY, NO_KEY, NO_KEY, NO_KEY, CODE_KEY(KEY_PRINT_SCREEN), CODE_KEY(KEY_SCROLL_LOCK), CODE_KEY(KEY_PAUSE), NO_KEY, NO_KEY},
  {NO_KEY, CODE_KEY(KEY_LEFT_ARROW), CODE_KEY(KEY_DOWN_ARROW), CODE_KEY(KEY_RIGHT_ARROW), NO_KEY, NO_KEY, NO_KEY, NO_KEY, NO_KEY, CODE_KEY(KEY_INSERT), CODE_KEY(KEY_HOME), CODE_KEY(KEY_PAGE_UP), CODE_KEY(KEY_RETURN), NO_KEY},
  {NO_KEY, NO_KEY, NO_KEY, NO_KEY, NO_KEY, NO_KEY, NO_KEY, NO_KEY, NO_KEY, CODE_KEY(KEY_DELETE), CODE_KEY(KEY_END), NO_KEY, NO_KEY, CODE_KEY(KEY_PAGE_DOWN)},
  {NO_KEY, NO_KEY, NO_KEY, NO_KEY, NO_KEY, NO_KEY, NO_KEY, NO_KEY, NO_KEY, NO_KEY, FN_KEY, NO_KEY, NO_KEY, NO_KEY},
};

bool last_usb_matrix[ROW_COUNT][FULL_COL_COUNT] = {};
bool last_fn_active = false;

bool keydef_equal(const KeyDef &left, const KeyDef &right) {
  return left.kind == right.kind && left.code == right.code;
}

const KeyDef &get_keydef(bool fn_active, size_t row, size_t col) {
  return fn_active ? FN_LAYER[row][col] : BASE_LAYER[row][col];
}


void press_keydef(const KeyDef &key) {
  switch (key.kind) {
    case KEY_KIND_CHAR:
      Keyboard.press(key.code);
      break;
    case KEY_KIND_CODE:
      if (key.code != 0) {
        Keyboard.press(key.code);
      }
      break;
    case KEY_KIND_CTRL_L:
      Keyboard.press(KEY_LEFT_CTRL);
      Keyboard.press('l');
      break;
    default:
      break;
  }
}

void release_keydef(const KeyDef &key) {
  switch (key.kind) {
    case KEY_KIND_CHAR:
      Keyboard.release(key.code);
      break;
    case KEY_KIND_CODE:
      if (key.code != 0) {
        Keyboard.release(key.code);
      }
      break;
    case KEY_KIND_CTRL_L:
      Keyboard.release('l');
      Keyboard.release(KEY_LEFT_CTRL);
      break;
    default:
      break;
  }
}

void build_usb_matrix(bool matrix_state[ROW_COUNT][FULL_COL_COUNT], const bool gpio_state[ROW_COUNT][COL_COUNT]) {
  for (size_t row = 0; row < ROW_COUNT; ++row) {
    for (size_t col = 0; col < COL_COUNT; ++col) {
      matrix_state[row][col] = gpio_state[row][col];
    }
  }
}

void rebuild_keyboard_report(const bool matrix_state[ROW_COUNT][FULL_COL_COUNT], bool fn_active) {
  Keyboard.releaseAll();

  for (size_t row = 0; row < ROW_COUNT; ++row) {
    for (size_t col = 0; col < FULL_COL_COUNT; ++col) {
      if (!matrix_state[row][col]) {
        continue;
      }
      if (row == FN_ROW && col == FN_COL) {
        continue;
      }

      press_keydef(get_keydef(fn_active, row, col));
    }
  }
}
#endif

unsigned long last_idle_report = 0;
unsigned long last_heartbeat = 0;
bool led_state = false;


#if ENABLE_RGB_TEST
Adafruit_NeoPixel rgb_pixels(RGB_TEST_LED_COUNT, RGB_TEST_PIN, NEO_GRB + NEO_KHZ800);
unsigned long last_rgb_update = 0;
uint8_t rgb_test_phase = 0;
uint16_t rgb_test_pixel = 0;
bool rgb_reactive = false;
unsigned long rgb_reactive_time = 0;

void fill_rgb(uint8_t red, uint8_t green, uint8_t blue) {
  for (uint16_t pixel = 0; pixel < RGB_TEST_LED_COUNT; ++pixel) {
    rgb_pixels.setPixelColor(pixel, rgb_pixels.Color(red, green, blue));
  }
  rgb_pixels.show();
}

void init_rgb_test() {
  rgb_pixels.begin();
  rgb_pixels.clear();
  rgb_pixels.setBrightness(32);
  rgb_pixels.show();

  fill_rgb(32, 0, 0);
  delay(250);
  fill_rgb(0, 32, 0);
  delay(250);
  fill_rgb(0, 0, 32);
  delay(250);
  rgb_pixels.clear();
  rgb_pixels.show();
}

void trigger_rgb_reactive() {
  rgb_reactive = true;
  rgb_reactive_time = millis();
  fill_rgb(64, 64, 64); // flash white
}

void update_rgb_test() {
  if (rgb_reactive) {
    if (millis() - rgb_reactive_time > 80) {
      rgb_reactive = false;
      // Resume idle animation
      rgb_test_phase = (rgb_test_phase + 1) % 4;
      rgb_test_pixel = 0;
    } else {
      return; // keep white for a short time
    }
  }

  if (millis() - last_rgb_update < 120) {
    return;
  }

  last_rgb_update = millis();
  rgb_pixels.clear();

  uint32_t color = 0;
  switch (rgb_test_phase) {
    case 0:
      color = rgb_pixels.Color(32, 0, 0);
      break;
    case 1:
      color = rgb_pixels.Color(0, 32, 0);
      break;
    case 2:
      color = rgb_pixels.Color(0, 0, 32);
      break;
    default:
      color = rgb_pixels.Color(24, 24, 24);
      break;
  }

  rgb_pixels.setPixelColor(rgb_test_pixel, color);
  rgb_pixels.show();

  ++rgb_test_pixel;
  if (rgb_test_pixel >= RGB_TEST_LED_COUNT) {
    rgb_test_pixel = 0;
    rgb_test_phase = (rgb_test_phase + 1) % 4;
  }
}
#endif

void set_all_inputs_pullup() {
  for (size_t row = 0; row < ROW_COUNT; ++row) {
    pinMode(ROW_PINS[row], INPUT_PULLUP);
  }

  for (size_t col = 0; col < COL_COUNT; ++col) {
    pinMode(COL_PINS[col], INPUT_PULLUP);
  }
}

void clear_matrix(bool matrix_state[ROW_COUNT][COL_COUNT]) {
  memset(matrix_state, 0, ROW_COUNT * COL_COUNT * sizeof(bool));
}

void clear_probe_matrix(bool matrix_state[ROW_COUNT][PROBE_COUNT]) {
  memset(matrix_state, 0, ROW_COUNT * PROBE_COUNT * sizeof(bool));
}

void scan_row2col(bool matrix_state[ROW_COUNT][COL_COUNT]) {
  clear_matrix(matrix_state);
  set_all_inputs_pullup();

  for (size_t col = 0; col < COL_COUNT; ++col) {
    pinMode(COL_PINS[col], OUTPUT);
    digitalWrite(COL_PINS[col], LOW);
    delayMicroseconds(30);

    for (size_t row = 0; row < ROW_COUNT; ++row) {
      matrix_state[row][col] = (digitalRead(ROW_PINS[row]) == LOW);
    }

    pinMode(COL_PINS[col], INPUT_PULLUP);
  }
}

void scan_col2row(bool matrix_state[ROW_COUNT][COL_COUNT]) {
  clear_matrix(matrix_state);
  set_all_inputs_pullup();

  for (size_t row = 0; row < ROW_COUNT; ++row) {
    pinMode(ROW_PINS[row], OUTPUT);
    digitalWrite(ROW_PINS[row], LOW);
    delayMicroseconds(30);

    for (size_t col = 0; col < COL_COUNT; ++col) {
      matrix_state[row][col] = (digitalRead(COL_PINS[col]) == LOW);
    }

    pinMode(ROW_PINS[row], INPUT_PULLUP);
  }
}

#if MATRIX_DIRECTION_COL2ROW
void scan_active_direction(bool matrix_state[ROW_COUNT][COL_COUNT]) {
  scan_col2row(matrix_state);
}

const char *active_direction_label() {
  return "COL2ROW";
}
#else
void scan_active_direction(bool matrix_state[ROW_COUNT][COL_COUNT]) {
  scan_row2col(matrix_state);
}

const char *active_direction_label() {
  return "ROW2COL";
}
#endif

#if !KEYBOARD_MODE
void scan_probe_row2col(bool matrix_state[ROW_COUNT][PROBE_COUNT]) {
  clear_probe_matrix(matrix_state);
  set_all_inputs_pullup();

  for (size_t probe = 0; probe < PROBE_COUNT; ++probe) {
    pinMode(PROBE_PINS[probe], OUTPUT);
    digitalWrite(PROBE_PINS[probe], LOW);
    delayMicroseconds(30);

    for (size_t row = 0; row < ROW_COUNT; ++row) {
      matrix_state[row][probe] = (digitalRead(ROW_PINS[row]) == LOW);
    }

    pinMode(PROBE_PINS[probe], INPUT_PULLUP);
  }
}

void scan_probe_col2row(bool matrix_state[ROW_COUNT][PROBE_COUNT]) {
  clear_probe_matrix(matrix_state);
  set_all_inputs_pullup();

  for (size_t row = 0; row < ROW_COUNT; ++row) {
    pinMode(ROW_PINS[row], OUTPUT);
    digitalWrite(ROW_PINS[row], LOW);
    delayMicroseconds(30);

    for (size_t probe = 0; probe < PROBE_COUNT; ++probe) {
      matrix_state[row][probe] = (digitalRead(PROBE_PINS[probe]) == LOW);
    }

    pinMode(ROW_PINS[row], INPUT_PULLUP);
  }
}

#endif

bool matrices_equal(const bool left[ROW_COUNT][COL_COUNT], const bool right[ROW_COUNT][COL_COUNT]) {
  return memcmp(left, right, ROW_COUNT * COL_COUNT * sizeof(bool)) == 0;
}

#if !KEYBOARD_MODE
bool probe_matrices_equal(const bool left[ROW_COUNT][PROBE_COUNT], const bool right[ROW_COUNT][PROBE_COUNT]) {
  return memcmp(left, right, ROW_COUNT * PROBE_COUNT * sizeof(bool)) == 0;
}
#endif

bool matrix_has_press(const bool matrix_state[ROW_COUNT][COL_COUNT]) {
  for (size_t row = 0; row < ROW_COUNT; ++row) {
    for (size_t col = 0; col < COL_COUNT; ++col) {
      if (matrix_state[row][col]) {
        return true;
      }
    }
  }

  return false;
}

#if !KEYBOARD_MODE
bool probe_matrix_has_press(const bool matrix_state[ROW_COUNT][PROBE_COUNT]) {
  for (size_t row = 0; row < ROW_COUNT; ++row) {
    for (size_t probe = 0; probe < PROBE_COUNT; ++probe) {
      if (matrix_state[row][probe]) {
        return true;
      }
    }
  }

  return false;
}
#endif

void copy_matrix(bool dst[ROW_COUNT][COL_COUNT], const bool src[ROW_COUNT][COL_COUNT]) {
  memcpy(dst, src, ROW_COUNT * COL_COUNT * sizeof(bool));
}

#if !KEYBOARD_MODE
void copy_probe_matrix(bool dst[ROW_COUNT][PROBE_COUNT], const bool src[ROW_COUNT][PROBE_COUNT]) {
  memcpy(dst, src, ROW_COUNT * PROBE_COUNT * sizeof(bool));
}
#endif

void print_matrix(const char *label, const bool matrix_state[ROW_COUNT][COL_COUNT]) {
  Serial.print(label);
  Serial.println(":");

  bool any = false;
  for (size_t row = 0; row < ROW_COUNT; ++row) {
    for (size_t col = 0; col < COL_COUNT; ++col) {
      if (matrix_state[row][col]) {
        any = true;
        Serial.print("  r");
        Serial.print(row);
        Serial.print(" c");
        Serial.print(col);
        Serial.print("  (GP");
        Serial.print(ROW_PINS[row]);
        Serial.print(" <-> GP");
        Serial.print(COL_PINS[col]);
        Serial.println(")");
      }
    }
  }

  if (!any) {
    Serial.println("  no keys detected");
  }
}

#if !KEYBOARD_MODE
void print_probe_matrix(const char *label, const bool matrix_state[ROW_COUNT][PROBE_COUNT]) {
  Serial.print(label);
  Serial.println(":");

  bool any = false;
  for (size_t row = 0; row < ROW_COUNT; ++row) {
    for (size_t probe = 0; probe < PROBE_COUNT; ++probe) {
      if (matrix_state[row][probe]) {
        any = true;
        Serial.print("  r");
        Serial.print(row);
        Serial.print(" probe");
        Serial.print(probe);
        Serial.print("  (GP");
        Serial.print(ROW_PINS[row]);
        Serial.print(" <-> GP");
        Serial.print(PROBE_PINS[probe]);
        Serial.println(")");
      }
    }
  }

  if (!any) {
    Serial.println("  no probe hits detected");
  }
}
#endif

void print_pin_summary() {
  Serial.println("Rows:");
  for (size_t row = 0; row < ROW_COUNT; ++row) {
    Serial.print("  r");
    Serial.print(row);
    Serial.print(" -> GP");
    Serial.println(ROW_PINS[row]);
  }

  Serial.println("Cols:");
  for (size_t col = 0; col < COL_COUNT; ++col) {
    Serial.print("  c");
    Serial.print(col);
    Serial.print(" -> GP");
    Serial.println(COL_PINS[col]);
  }

  Serial.println("Probe pins:");
  for (size_t probe = 0; probe < PROBE_COUNT; ++probe) {
    Serial.print("  p");
    Serial.print(probe);
    Serial.print(" -> GP");
    Serial.println(PROBE_PINS[probe]);
  }

#if ENABLE_USB_KEYBOARD
#if KEYBOARD_MODE
  Serial.println("USB keyboard mode: enabled (keyboard-first scan)");
#else
  Serial.println("USB keyboard mode: enabled (diagnostic scan)");
#endif
  Serial.println("Bottom row and Fn placement are a first-pass ANSI estimate.");
#else
  Serial.println("USB keyboard mode: Keyboard.h not available in this Arduino core");
#endif
}


#if ENABLE_USB_KEYBOARD
void update_usb_keyboard(const bool gpio_matrix[ROW_COUNT][COL_COUNT]) {
  bool current_usb_matrix[ROW_COUNT][FULL_COL_COUNT];
  build_usb_matrix(current_usb_matrix, gpio_matrix);

  const bool fn_active = current_usb_matrix[FN_ROW][FN_COL];
  bool key_pressed = false;
  if (fn_active != last_fn_active) {
    rebuild_keyboard_report(current_usb_matrix, fn_active);
    key_pressed = true;
  } else {
    for (size_t row = 0; row < ROW_COUNT; ++row) {
      for (size_t col = 0; col < FULL_COL_COUNT; ++col) {
        if (row == FN_ROW && col == FN_COL) {
          continue;
        }

        const KeyDef &active_key = get_keydef(fn_active, row, col);
        const KeyDef &previous_key = get_keydef(last_fn_active, row, col);

        if (current_usb_matrix[row][col] && !last_usb_matrix[row][col]) {
          press_keydef(active_key);
          key_pressed = true;
        } else if (!current_usb_matrix[row][col] && last_usb_matrix[row][col]) {
          release_keydef(previous_key);
        }
      }
    }
  }

#if ENABLE_RGB_TEST
  if (key_pressed) {
    trigger_rgb_reactive();
  }
#endif

  memcpy(last_usb_matrix, current_usb_matrix, sizeof(last_usb_matrix));
  last_fn_active = fn_active;
}
#endif

void setup() {
  #ifdef LED_BUILTIN
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
  #endif

#if ENABLE_RGB_TEST
  init_rgb_test();
#endif

  set_all_inputs_pullup();

#if ENABLE_USB_KEYBOARD
  Keyboard.begin();
#endif

  Serial.begin(115200);
  unsigned long start = millis();
  while (!Serial && (millis() - start) < 4000) {
  }

  Serial.println();
  Serial.println("RP2040 matrix diagnostic");
#if KEYBOARD_MODE
    Serial.println("Keyboard mode is active.");
  Serial.print("The sketch scans the intended ");
  Serial.print(active_direction_label());
  Serial.println(" matrix and sends USB key events.");
#else
    Serial.println("This sketch scans both diode directions.");
    Serial.println("Press keys and watch which mode reports coordinates.");
    Serial.println("If a missing column is wired to an unknown GPIO, check the probe output.");
#endif
  print_pin_summary();
#if ENABLE_RGB_TEST
  Serial.print("RGB test: enabled on GP");
  Serial.print(RGB_TEST_PIN);
  Serial.print(" for ");
  Serial.print(RGB_TEST_LED_COUNT);
  Serial.println(" WS2812 LEDs");
  Serial.println("Power the LED strip from 5V with shared ground; 3.3V is not enough for a 36-pixel strip.");
#elif RGB_TEST_ENABLE
  Serial.println("RGB test: Adafruit_NeoPixel library not found, so GP29 LED test is disabled.");
#endif
  Serial.println();
}

void loop() {
#if ENABLE_RGB_TEST
  update_rgb_test();
#endif

  bool current_matrix[ROW_COUNT][COL_COUNT];

  scan_active_direction(current_matrix);

#if ENABLE_USB_KEYBOARD
  update_usb_keyboard(current_matrix);
#endif

#if KEYBOARD_MODE
  const bool matrix_changed = !matrices_equal(current_matrix, last_row2col);
  const bool matrix_active = matrix_has_press(current_matrix);

  if (matrix_changed) {
    Serial.println("------------------------------");
    print_matrix(active_direction_label(), current_matrix);
    Serial.println();
  } else if (!matrix_active && millis() - last_idle_report > 2000) {
    Serial.println("idle: no keys detected in active keyboard scan");
    last_idle_report = millis();
  }

  copy_matrix(last_row2col, current_matrix);

  #ifdef LED_BUILTIN
  if (millis() - last_heartbeat > 500) {
    led_state = !led_state;
    digitalWrite(LED_BUILTIN, led_state ? HIGH : LOW);
    last_heartbeat = millis();
  }
  #endif

  delay(5);
#else
  bool current_col2row[ROW_COUNT][COL_COUNT];
  bool current_probe_row2col[ROW_COUNT][PROBE_COUNT];
  bool current_probe_col2row[ROW_COUNT][PROBE_COUNT];

  scan_col2row(current_col2row);
  scan_probe_row2col(current_probe_row2col);
  scan_probe_col2row(current_probe_col2row);

  const bool row2col_changed = !matrices_equal(current_row2col, last_row2col);
  const bool col2row_changed = !matrices_equal(current_col2row, last_col2row);
  const bool probe_row2col_changed = !probe_matrices_equal(current_probe_row2col, last_probe_row2col);
  const bool probe_col2row_changed = !probe_matrices_equal(current_probe_col2row, last_probe_col2row);
  const bool row2col_active = matrix_has_press(current_row2col);
  const bool col2row_active = matrix_has_press(current_col2row);
  const bool probe_row2col_active = probe_matrix_has_press(current_probe_row2col);
  const bool probe_col2row_active = probe_matrix_has_press(current_probe_col2row);

  if (row2col_changed || col2row_changed || probe_row2col_changed || probe_col2row_changed) {
    Serial.println("------------------------------");
    if (row2col_changed || row2col_active) {
      print_matrix("ROW2COL", current_row2col);
    }
    if (col2row_changed || col2row_active) {
      print_matrix("COL2ROW", current_col2row);
    }
    if (probe_row2col_changed || probe_row2col_active) {
      print_probe_matrix("PROBE ROW2COL", current_probe_row2col);
    }
    if (probe_col2row_changed || probe_col2row_active) {
      print_probe_matrix("PROBE COL2ROW", current_probe_col2row);
    }
    Serial.println();
  } else if (!row2col_active && !col2row_active && !probe_row2col_active && !probe_col2row_active && millis() - last_idle_report > 2000) {
    Serial.println("idle: no keys detected in configured pins or probe pins");
    last_idle_report = millis();
  }

  copy_matrix(last_row2col, current_row2col);
  copy_matrix(last_col2row, current_col2row);
  copy_probe_matrix(last_probe_row2col, current_probe_row2col);
  copy_probe_matrix(last_probe_col2row, current_probe_col2row);

  #ifdef LED_BUILTIN
  if (millis() - last_heartbeat > 500) {
    led_state = !led_state;
    digitalWrite(LED_BUILTIN, led_state ? HIGH : LOW);
    last_heartbeat = millis();
  }
  #endif

  delay(25);
#endif
}

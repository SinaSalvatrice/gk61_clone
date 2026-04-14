#include <Arduino.h>
#include <Wire.h>

#if __has_include(<Keyboard.h>)
#include <Keyboard.h>
#define ENABLE_USB_KEYBOARD 1
#else
#define ENABLE_USB_KEYBOARD 0
#endif

constexpr uint8_t ROW_PINS[] = {2, 3, 4, 5, 6};
constexpr uint8_t COL_PINS[] = {7, 8, 9, 10, 11, 12, 13, 14, 15, 26, 27, 28, 29};
constexpr uint8_t PROBE_PINS[] = {0, 1, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25};

constexpr bool USE_PCF8575_LAST_COL = true;
constexpr bool KEYBOARD_MODE = true;
constexpr uint8_t PCF8575_ADDR = 0x20;
constexpr int8_t I2C_SDA_PIN = 0;
constexpr int8_t I2C_SCL_PIN = 1;
constexpr uint16_t PCF8575_ALL_HIGH = 0xFFFF;
constexpr uint16_t PCF8575_P10_MASK = 0x0100;

const uint16_t PCF8575_PIN_MASKS[] = {
  0x0001, 0x0002, 0x0004, 0x0008, 0x0010, 0x0020, 0x0040, 0x0080,
  0x0100, 0x0200, 0x0400, 0x0800, 0x1000, 0x2000, 0x4000, 0x8000,
};

const char *const PCF8575_PIN_LABELS[] = {
  "P00", "P01", "P02", "P03", "P04", "P05", "P06", "P07",
  "P10", "P11", "P12", "P13", "P14", "P15", "P16", "P17",
};

constexpr size_t PCF8575_PIN_COUNT = sizeof(PCF8575_PIN_MASKS) / sizeof(PCF8575_PIN_MASKS[0]);

struct I2CPinPair {
  uint8_t sda;
  uint8_t scl;
};

const I2CPinPair RP2040_I2C_PAIRS[] = {
  {0, 1}, {4, 5}, {8, 9}, {12, 13}, {16, 17}, {20, 21},
  {2, 3}, {6, 7}, {10, 11}, {14, 15}, {18, 19}, {26, 27},
};

constexpr size_t I2C_PAIR_COUNT = sizeof(RP2040_I2C_PAIRS) / sizeof(RP2040_I2C_PAIRS[0]);

constexpr size_t ROW_COUNT = sizeof(ROW_PINS) / sizeof(ROW_PINS[0]);
constexpr size_t COL_COUNT = sizeof(COL_PINS) / sizeof(COL_PINS[0]);
constexpr size_t PROBE_COUNT = sizeof(PROBE_PINS) / sizeof(PROBE_PINS[0]);
constexpr size_t FULL_COL_COUNT = COL_COUNT + 1;

bool last_row2col[ROW_COUNT][COL_COUNT] = {};
bool last_col2row[ROW_COUNT][COL_COUNT] = {};
bool last_probe_row2col[ROW_COUNT][PROBE_COUNT] = {};
bool last_probe_col2row[ROW_COUNT][PROBE_COUNT] = {};
bool last_expander_row2col[ROW_COUNT] = {};
bool last_expander_col2row[ROW_COUNT] = {};
bool last_expander_probe[PCF8575_PIN_COUNT][ROW_COUNT] = {};

#if ENABLE_USB_KEYBOARD
enum KeyKind : uint8_t {
  KEY_KIND_NONE,
  KEY_KIND_CHAR,
  KEY_KIND_CODE,
  KEY_KIND_FN,
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

#ifndef KEY_MENU
#define KEY_MENU 0
#endif

constexpr size_t FN_ROW = 4;
constexpr size_t FN_COL = 10;

const KeyDef BASE_LAYER[ROW_COUNT][FULL_COL_COUNT] = {
  {CODE_KEY(KEY_ESC), CODE_KEY(HID_KEY(0x1E)), CODE_KEY(HID_KEY(0x1F)), CODE_KEY(HID_KEY(0x20)), CODE_KEY(HID_KEY(0x21)), CODE_KEY(HID_KEY(0x22)), CODE_KEY(HID_KEY(0x23)), CODE_KEY(HID_KEY(0x24)), CODE_KEY(HID_KEY(0x25)), CODE_KEY(HID_KEY(0x26)), CODE_KEY(HID_KEY(0x27)), CODE_KEY(HID_KEY(0x2D)), CODE_KEY(HID_KEY(0x2E)), CODE_KEY(KEY_BACKSPACE)},
  {CODE_KEY(KEY_TAB), CODE_KEY(HID_KEY(0x14)), CODE_KEY(HID_KEY(0x1A)), CODE_KEY(HID_KEY(0x08)), CODE_KEY(HID_KEY(0x15)), CODE_KEY(HID_KEY(0x17)), CODE_KEY(HID_KEY(0x1C)), CODE_KEY(HID_KEY(0x18)), CODE_KEY(HID_KEY(0x0C)), CODE_KEY(HID_KEY(0x12)), CODE_KEY(HID_KEY(0x13)), CODE_KEY(HID_KEY(0x2F)), CODE_KEY(HID_KEY(0x30)), CODE_KEY(HID_KEY(0x31))},
  {CODE_KEY(KEY_CAPS_LOCK), CODE_KEY(HID_KEY(0x04)), CODE_KEY(HID_KEY(0x16)), CODE_KEY(HID_KEY(0x07)), CODE_KEY(HID_KEY(0x09)), CODE_KEY(HID_KEY(0x0A)), CODE_KEY(HID_KEY(0x0B)), CODE_KEY(HID_KEY(0x0D)), CODE_KEY(HID_KEY(0x0E)), CODE_KEY(HID_KEY(0x0F)), CODE_KEY(HID_KEY(0x33)), CODE_KEY(HID_KEY(0x34)), CODE_KEY(KEY_RETURN), NO_KEY},
  {CODE_KEY(KEY_LEFT_SHIFT), CODE_KEY(HID_KEY(0x1D)), CODE_KEY(HID_KEY(0x1B)), CODE_KEY(HID_KEY(0x06)), CODE_KEY(HID_KEY(0x19)), CODE_KEY(HID_KEY(0x05)), CODE_KEY(HID_KEY(0x11)), CODE_KEY(HID_KEY(0x10)), CODE_KEY(HID_KEY(0x36)), CODE_KEY(HID_KEY(0x37)), CODE_KEY(HID_KEY(0x38)), NO_KEY, NO_KEY, CODE_KEY(KEY_RIGHT_SHIFT)},
  {CODE_KEY(KEY_LEFT_CTRL), CODE_KEY(KEY_LEFT_GUI), CODE_KEY(KEY_LEFT_ALT), NO_KEY, NO_KEY, CODE_KEY(HID_KEY(0x2C)), NO_KEY, NO_KEY, NO_KEY, CODE_KEY(KEY_RIGHT_ALT), FN_KEY, CODE_KEY(KEY_MENU), NO_KEY, CODE_KEY(KEY_RIGHT_CTRL)},
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
    default:
      break;
  }
}

void build_usb_matrix(bool matrix_state[ROW_COUNT][FULL_COL_COUNT], const bool gpio_state[ROW_COUNT][COL_COUNT], const bool expander_state[ROW_COUNT]) {
  for (size_t row = 0; row < ROW_COUNT; ++row) {
    for (size_t col = 0; col < COL_COUNT; ++col) {
      matrix_state[row][col] = gpio_state[row][col];
    }
    matrix_state[row][COL_COUNT] = expander_state[row];
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
bool expander_present = false;
uint8_t detected_expander_addr = PCF8575_ADDR;
uint16_t pcf8575_output_state = PCF8575_ALL_HIGH;
int8_t detected_sda_pin = I2C_SDA_PIN;
int8_t detected_scl_pin = I2C_SCL_PIN;

void configure_wire_pins(uint8_t sda, uint8_t scl) {
#if defined(ARDUINO_ARCH_RP2040)
  Wire.end();
  Wire.setSDA(sda);
  Wire.setSCL(scl);
#endif
  Wire.begin();
}

bool i2c_device_present(uint8_t addr) {
  Wire.beginTransmission(addr);
  return Wire.endTransmission() == 0;
}

uint8_t find_pcf8575_addr() {
  for (uint8_t addr = 0x20; addr <= 0x27; ++addr) {
    if (i2c_device_present(addr)) {
      return addr;
    }
  }

  return 0;
}

bool detect_expander_bus() {
  if (!USE_PCF8575_LAST_COL) {
    return false;
  }

  if (I2C_SDA_PIN >= 0 && I2C_SCL_PIN >= 0) {
    configure_wire_pins(I2C_SDA_PIN, I2C_SCL_PIN);
    detected_expander_addr = find_pcf8575_addr();
    if (detected_expander_addr != 0) {
      detected_sda_pin = I2C_SDA_PIN;
      detected_scl_pin = I2C_SCL_PIN;
      return true;
    }
  }

  for (size_t index = 0; index < I2C_PAIR_COUNT; ++index) {
    configure_wire_pins(RP2040_I2C_PAIRS[index].sda, RP2040_I2C_PAIRS[index].scl);
    detected_expander_addr = find_pcf8575_addr();
    if (detected_expander_addr != 0) {
      detected_sda_pin = static_cast<int8_t>(RP2040_I2C_PAIRS[index].sda);
      detected_scl_pin = static_cast<int8_t>(RP2040_I2C_PAIRS[index].scl);
      return true;
    }
  }

  return false;
}

bool pcf8575_write_state(uint16_t value) {
  Wire.beginTransmission(detected_expander_addr);
  Wire.write(static_cast<uint8_t>(value & 0xFF));
  Wire.write(static_cast<uint8_t>((value >> 8) & 0xFF));
  return Wire.endTransmission() == 0;
}

bool pcf8575_read_state(uint16_t &value) {
  if (Wire.requestFrom(static_cast<int>(detected_expander_addr), 2) != 2) {
    return false;
  }

  const uint8_t low = Wire.read();
  const uint8_t high = Wire.read();
  value = static_cast<uint16_t>(low) | (static_cast<uint16_t>(high) << 8);
  return true;
}

bool pcf8575_release_p10() {
  if (!USE_PCF8575_LAST_COL) {
    return false;
  }

  pcf8575_output_state |= PCF8575_P10_MASK;
  return pcf8575_write_state(pcf8575_output_state);
}

bool pcf8575_drive_p10_low() {
  if (!USE_PCF8575_LAST_COL) {
    return false;
  }

  pcf8575_output_state &= ~PCF8575_P10_MASK;
  return pcf8575_write_state(pcf8575_output_state);
}

bool pcf8575_read_p10_pressed(bool &pressed) {
  uint16_t input_state = PCF8575_ALL_HIGH;
  if (!pcf8575_read_state(input_state)) {
    return false;
  }

  pressed = (input_state & PCF8575_P10_MASK) == 0;
  return true;
}

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

void clear_expander_column(bool column_state[ROW_COUNT]) {
  memset(column_state, 0, ROW_COUNT * sizeof(bool));
}

void clear_expander_probe(bool probe_state[PCF8575_PIN_COUNT][ROW_COUNT]) {
  memset(probe_state, 0, PCF8575_PIN_COUNT * ROW_COUNT * sizeof(bool));
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

void scan_expander_row2col(bool column_state[ROW_COUNT]) {
  clear_expander_column(column_state);

  if (!expander_present) {
    return;
  }

  set_all_inputs_pullup();
  if (!pcf8575_drive_p10_low()) {
    expander_present = false;
    return;
  }

  delayMicroseconds(30);
  for (size_t row = 0; row < ROW_COUNT; ++row) {
    column_state[row] = (digitalRead(ROW_PINS[row]) == LOW);
  }

  if (!pcf8575_release_p10()) {
    expander_present = false;
  }
}

void scan_expander_col2row(bool column_state[ROW_COUNT]) {
  clear_expander_column(column_state);

  if (!expander_present) {
    return;
  }

  set_all_inputs_pullup();
  if (!pcf8575_release_p10()) {
    expander_present = false;
    return;
  }

  for (size_t row = 0; row < ROW_COUNT; ++row) {
    pinMode(ROW_PINS[row], OUTPUT);
    digitalWrite(ROW_PINS[row], LOW);
    delayMicroseconds(30);

    bool pressed = false;
    if (!pcf8575_read_p10_pressed(pressed)) {
      expander_present = false;
      pinMode(ROW_PINS[row], INPUT_PULLUP);
      return;
    }
    column_state[row] = pressed;

    pinMode(ROW_PINS[row], INPUT_PULLUP);
  }
}

void scan_expander_probe_row2col(bool probe_state[PCF8575_PIN_COUNT][ROW_COUNT]) {
  clear_expander_probe(probe_state);

  if (!expander_present) {
    return;
  }

  set_all_inputs_pullup();

  for (size_t pin = 0; pin < PCF8575_PIN_COUNT; ++pin) {
    pcf8575_output_state = PCF8575_ALL_HIGH & ~PCF8575_PIN_MASKS[pin];
    if (!pcf8575_write_state(pcf8575_output_state)) {
      expander_present = false;
      return;
    }

    delayMicroseconds(30);
    for (size_t row = 0; row < ROW_COUNT; ++row) {
      probe_state[pin][row] = (digitalRead(ROW_PINS[row]) == LOW);
    }
  }

  pcf8575_output_state = PCF8575_ALL_HIGH;
  if (!pcf8575_write_state(pcf8575_output_state)) {
    expander_present = false;
  }
}

bool matrices_equal(const bool left[ROW_COUNT][COL_COUNT], const bool right[ROW_COUNT][COL_COUNT]) {
  return memcmp(left, right, ROW_COUNT * COL_COUNT * sizeof(bool)) == 0;
}

bool probe_matrices_equal(const bool left[ROW_COUNT][PROBE_COUNT], const bool right[ROW_COUNT][PROBE_COUNT]) {
  return memcmp(left, right, ROW_COUNT * PROBE_COUNT * sizeof(bool)) == 0;
}

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

bool expander_column_has_press(const bool column_state[ROW_COUNT]) {
  for (size_t row = 0; row < ROW_COUNT; ++row) {
    if (column_state[row]) {
      return true;
    }
  }

  return false;
}

void copy_matrix(bool dst[ROW_COUNT][COL_COUNT], const bool src[ROW_COUNT][COL_COUNT]) {
  memcpy(dst, src, ROW_COUNT * COL_COUNT * sizeof(bool));
}

void copy_probe_matrix(bool dst[ROW_COUNT][PROBE_COUNT], const bool src[ROW_COUNT][PROBE_COUNT]) {
  memcpy(dst, src, ROW_COUNT * PROBE_COUNT * sizeof(bool));
}

void copy_expander_column(bool dst[ROW_COUNT], const bool src[ROW_COUNT]) {
  memcpy(dst, src, ROW_COUNT * sizeof(bool));
}

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

void print_expander_column(const char *label, const bool column_state[ROW_COUNT]) {
  Serial.print(label);
  Serial.println(":");

  bool any = false;
  for (size_t row = 0; row < ROW_COUNT; ++row) {
    if (column_state[row]) {
      any = true;
      Serial.print("  r");
      Serial.print(row);
      Serial.println(" c13  (expander P10)");
    }
  }

  if (!any) {
    if (expander_present) {
      Serial.println("  no expander hits detected");
    } else {
      Serial.println("  expander not responding");
    }
  }
}

void print_expander_probe(const bool probe_state[PCF8575_PIN_COUNT][ROW_COUNT]) {
  Serial.println("EXPANDER PROBE:");

  bool any = false;
  for (size_t pin = 0; pin < PCF8575_PIN_COUNT; ++pin) {
    for (size_t row = 0; row < ROW_COUNT; ++row) {
      if (probe_state[pin][row]) {
        any = true;
        Serial.print("  ");
        Serial.print(PCF8575_PIN_LABELS[pin]);
        Serial.print(" -> r");
        Serial.println(row);
      }
    }
  }

  if (!any) {
    if (expander_present) {
      Serial.println("  no expander pin caused any row activity");
    } else {
      Serial.println("  expander not responding");
    }
  }
}

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

  if (USE_PCF8575_LAST_COL) {
    Serial.print("Extra column: c13 -> PCF8575 P10, expected default 0x");
    Serial.println(PCF8575_ADDR, HEX);
    Serial.print("Configured I2C pins: SDA=");
    Serial.print(I2C_SDA_PIN);
    Serial.print(" SCL=");
    Serial.println(I2C_SCL_PIN);
  }

#if ENABLE_USB_KEYBOARD
  Serial.println(KEYBOARD_MODE ? "USB keyboard mode: enabled (keyboard-first scan)" : "USB keyboard mode: enabled (diagnostic scan)");
  Serial.println("Bottom row and Fn placement are a first-pass ANSI estimate.");
#else
  Serial.println("USB keyboard mode: Keyboard.h not available in this Arduino core");
#endif
}

#if ENABLE_USB_KEYBOARD
void update_usb_keyboard(const bool gpio_matrix[ROW_COUNT][COL_COUNT], const bool expander_state[ROW_COUNT]) {
  bool current_usb_matrix[ROW_COUNT][FULL_COL_COUNT];
  build_usb_matrix(current_usb_matrix, gpio_matrix, expander_state);

  const bool fn_active = current_usb_matrix[FN_ROW][FN_COL];
  if (fn_active != last_fn_active) {
    rebuild_keyboard_report(current_usb_matrix, fn_active);
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
        } else if (!current_usb_matrix[row][col] && last_usb_matrix[row][col]) {
          release_keydef(previous_key);
        }
      }
    }
  }

  memcpy(last_usb_matrix, current_usb_matrix, sizeof(last_usb_matrix));
  last_fn_active = fn_active;
}
#endif

void setup() {
  #ifdef LED_BUILTIN
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
  #endif

  set_all_inputs_pullup();

  if (I2C_SDA_PIN >= 0 && I2C_SCL_PIN >= 0) {
    configure_wire_pins(I2C_SDA_PIN, I2C_SCL_PIN);
  } else {
    Wire.begin();
  }

  if (USE_PCF8575_LAST_COL) {
    detect_expander_bus();
    pcf8575_output_state = PCF8575_ALL_HIGH;
    expander_present = detected_expander_addr != 0 && pcf8575_release_p10();
  }

#if ENABLE_USB_KEYBOARD
  Keyboard.begin();
#endif

  Serial.begin(115200);
  unsigned long start = millis();
  while (!Serial && (millis() - start) < 4000) {
  }

  Serial.println();
  Serial.println("RP2040 matrix diagnostic");
  if (KEYBOARD_MODE) {
    Serial.println("Keyboard mode is active.");
    Serial.println("The sketch scans the intended ROW2COL matrix and sends USB key events.");
  } else {
    Serial.println("This sketch scans both diode directions.");
    Serial.println("Press keys and watch which mode reports coordinates.");
    Serial.println("If a missing column is wired to an unknown GPIO, check the probe output.");
  }
  if (USE_PCF8575_LAST_COL) {
    if (expander_present) {
      Serial.print("Detected expander at 0x");
      Serial.println(detected_expander_addr, HEX);
      Serial.print("Detected I2C pins: SDA=GP");
      Serial.print(detected_sda_pin);
      Serial.print(" SCL=GP");
      Serial.println(detected_scl_pin);
    } else {
      Serial.println("No PCF8575 found at 0x20-0x27 on common RP2040 I2C pin pairs.");
    }
  }
  print_pin_summary();
  Serial.println();
}

void loop() {
  bool current_row2col[ROW_COUNT][COL_COUNT];
  bool current_expander_row2col[ROW_COUNT];

  scan_row2col(current_row2col);
  scan_expander_row2col(current_expander_row2col);

#if ENABLE_USB_KEYBOARD
  update_usb_keyboard(current_row2col, current_expander_row2col);
#endif

  if (KEYBOARD_MODE) {
    const bool row2col_changed = !matrices_equal(current_row2col, last_row2col);
    const bool expander_row2col_changed = memcmp(current_expander_row2col, last_expander_row2col, ROW_COUNT * sizeof(bool)) != 0;
    const bool row2col_active = matrix_has_press(current_row2col);
    const bool expander_row2col_active = expander_column_has_press(current_expander_row2col);

    if (row2col_changed || expander_row2col_changed) {
      Serial.println("------------------------------");
      print_matrix("ROW2COL", current_row2col);
      print_expander_column("EXPANDER ROW2COL", current_expander_row2col);
      Serial.println();
    } else if (!row2col_active && !expander_row2col_active && millis() - last_idle_report > 2000) {
      Serial.println("idle: no keys detected in active keyboard scan");
      last_idle_report = millis();
    }

    copy_matrix(last_row2col, current_row2col);
    copy_expander_column(last_expander_row2col, current_expander_row2col);

    #ifdef LED_BUILTIN
    if (millis() - last_heartbeat > 500) {
      led_state = !led_state;
      digitalWrite(LED_BUILTIN, led_state ? HIGH : LOW);
      last_heartbeat = millis();
    }
    #endif

    delay(5);
    return;
  }

  bool current_col2row[ROW_COUNT][COL_COUNT];
  bool current_probe_row2col[ROW_COUNT][PROBE_COUNT];
  bool current_probe_col2row[ROW_COUNT][PROBE_COUNT];
  bool current_expander_col2row[ROW_COUNT];
  bool current_expander_probe[PCF8575_PIN_COUNT][ROW_COUNT];

  scan_col2row(current_col2row);
  scan_probe_row2col(current_probe_row2col);
  scan_probe_col2row(current_probe_col2row);
  scan_expander_col2row(current_expander_col2row);
  scan_expander_probe_row2col(current_expander_probe);

  const bool row2col_changed = !matrices_equal(current_row2col, last_row2col);
  const bool col2row_changed = !matrices_equal(current_col2row, last_col2row);
  const bool probe_row2col_changed = !probe_matrices_equal(current_probe_row2col, last_probe_row2col);
  const bool probe_col2row_changed = !probe_matrices_equal(current_probe_col2row, last_probe_col2row);
  const bool expander_row2col_changed = memcmp(current_expander_row2col, last_expander_row2col, ROW_COUNT * sizeof(bool)) != 0;
  const bool expander_col2row_changed = memcmp(current_expander_col2row, last_expander_col2row, ROW_COUNT * sizeof(bool)) != 0;
  const bool expander_probe_changed = memcmp(current_expander_probe, last_expander_probe, sizeof(last_expander_probe)) != 0;
  const bool row2col_active = matrix_has_press(current_row2col);
  const bool col2row_active = matrix_has_press(current_col2row);
  const bool probe_row2col_active = probe_matrix_has_press(current_probe_row2col);
  const bool probe_col2row_active = probe_matrix_has_press(current_probe_col2row);
  const bool expander_row2col_active = expander_column_has_press(current_expander_row2col);
  const bool expander_col2row_active = expander_column_has_press(current_expander_col2row);

#if ENABLE_USB_KEYBOARD
  update_usb_keyboard(current_row2col, current_expander_row2col);
#endif

  if (row2col_changed || col2row_changed || probe_row2col_changed || probe_col2row_changed || expander_row2col_changed || expander_col2row_changed || expander_probe_changed) {
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
    if (expander_row2col_changed || expander_row2col_active) {
      print_expander_column("EXPANDER ROW2COL", current_expander_row2col);
    }
    if (expander_col2row_changed || expander_col2row_active) {
      print_expander_column("EXPANDER COL2ROW", current_expander_col2row);
    }
    if (expander_probe_changed) {
      print_expander_probe(current_expander_probe);
    }
    Serial.println();
  } else if (!row2col_active && !col2row_active && !probe_row2col_active && !probe_col2row_active && !expander_row2col_active && !expander_col2row_active && millis() - last_idle_report > 2000) {
    Serial.println("idle: no keys detected in configured pins, expander column, or probe pins");
    last_idle_report = millis();
  }

  copy_matrix(last_row2col, current_row2col);
  copy_matrix(last_col2row, current_col2row);
  copy_probe_matrix(last_probe_row2col, current_probe_row2col);
  copy_probe_matrix(last_probe_col2row, current_probe_col2row);
  copy_expander_column(last_expander_row2col, current_expander_row2col);
  copy_expander_column(last_expander_col2row, current_expander_col2row);
  memcpy(last_expander_probe, current_expander_probe, sizeof(last_expander_probe));

  #ifdef LED_BUILTIN
  if (millis() - last_heartbeat > 500) {
    led_state = !led_state;
    digitalWrite(LED_BUILTIN, led_state ? HIGH : LOW);
    last_heartbeat = millis();
  }
  #endif

  delay(25);
}

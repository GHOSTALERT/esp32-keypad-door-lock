// Pin map, password and timing for the ESP32 door lock.
#pragma once

#include <Arduino.h>

// ---- Security ----
#define LOCK_PASSWORD        "2580"   // change before deploying
#define MAX_PASSWORD_LEN     8
#define MAX_FAILED_ATTEMPTS  4
#define LOCKOUT_MS           30000UL  // 30 s lockout after MAX_FAILED_ATTEMPTS
#define UNLOCK_MS            5000UL   // solenoid held open for 5 s
#define MESSAGE_MS           1500UL   // "Wrong password" screen time

// ---- Keypad (4x4 membrane) ----
#define KEYPAD_ROWS 4
#define KEYPAD_COLS 4
static const uint8_t ROW_PINS[KEYPAD_ROWS] = {13, 12, 14, 27};
static const uint8_t COL_PINS[KEYPAD_COLS] = {26, 25, 33, 32};
static const char KEYMAP[KEYPAD_ROWS][KEYPAD_COLS] = {
    {'1', '2', '3', 'A'},
    {'4', '5', '6', 'B'},
    {'7', '8', '9', 'C'},
    {'*', '0', '#', 'D'},
};
// '#' = enter, '*' = clear, 'D' = delete last digit, 'A'-'C' unused

// ---- OLED (SSD1306 128x64, I2C) ----
#define OLED_SDA      21
#define OLED_SCL      22
#define OLED_ADDR     0x3C
#define OLED_WIDTH    128
#define OLED_HEIGHT   64

// ---- Outputs ----
#define RELAY_PIN     4
#define RELAY_ON      HIGH     // most opto-isolated relay boards are active LOW: set LOW then
#define LED_GREEN_PIN 18
#define LED_RED_PIN   19
#define BUZZER_PIN    23

// ---- Loop ----
#define SCAN_PERIOD_MS 10      // ~100 Hz keypad polling

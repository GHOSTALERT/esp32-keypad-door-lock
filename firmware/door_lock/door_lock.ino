// Microcontroller-based digital door lock with security alert (ESP32)
//
// 4x4 keypad -> password check -> relay -> 12 V solenoid lock.
// SSD1306 OLED for prompts, green/red LEDs for status, buzzer for audio.
// Four wrong passwords in a row lock the keypad for 30 s.
//
// Libraries: Keypad (Mark Stanley, Alexander Brevig), Adafruit SSD1306,
// Adafruit GFX. Board: ESP32 Dev Module.

#include <Wire.h>
#include <Keypad.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include "config.h"
#include "lock_logic.h"
#include "buzzer.h"

Keypad keypad(makeKeymap(KEYMAP), (byte *)ROW_PINS, (byte *)COL_PINS, KEYPAD_ROWS, KEYPAD_COLS);
Adafruit_SSD1306 oled(OLED_WIDTH, OLED_HEIGHT, &Wire, -1);
Buzzer buzzer(BUZZER_PIN);
LockLogic lock(LOCK_PASSWORD, MAX_FAILED_ATTEMPTS, UNLOCK_MS, LOCKOUT_MS, MESSAGE_MS, MAX_PASSWORD_LEN);

bool oled_ok = false;
bool redraw = true;
uint32_t last_countdown = 0;

// ------------------------------------------------------------ outputs

void setOutputs() {
    bool open = lock.relay_on();
    digitalWrite(RELAY_PIN, open ? RELAY_ON : !RELAY_ON);
    digitalWrite(LED_GREEN_PIN, open ? HIGH : LOW);
    bool red = lock.state() == LockLogic::WRONG || lock.state() == LockLogic::LOCKOUT;
    digitalWrite(LED_RED_PIN, red ? HIGH : LOW);
}

void centered(const char *text, int16_t y, uint8_t size) {
    int16_t x1, y1;
    uint16_t w, h;
    oled.setTextSize(size);
    oled.getTextBounds(text, 0, y, &x1, &y1, &w, &h);
    oled.setCursor((OLED_WIDTH - w) / 2, y);
    oled.print(text);
}

void drawScreen(uint32_t now) {
    if (!oled_ok) return;
    char line[24];
    oled.clearDisplay();
    oled.setTextColor(SSD1306_WHITE);

    switch (lock.state()) {
    case LockLogic::ENTRY: {
        centered("ENTER PASSWORD", 0, 1);
        char stars[MAX_PASSWORD_LEN + 1] = {0};
        memset(stars, '*', lock.entered());
        centered(lock.entered() ? stars : "_", 22, 2);
        snprintf(line, sizeof(line), "#=OK *=CLR D=DEL");
        centered(line, 56, 1);
        break;
    }
    case LockLogic::UNLOCKED:
        centered("ACCESS", 4, 2);
        centered("GRANTED", 22, 2);
        snprintf(line, sizeof(line), "Locking in %lus", (unsigned long)lock.seconds_left(now));
        centered(line, 50, 1);
        break;
    case LockLogic::WRONG:
        centered("WRONG", 4, 2);
        centered("PASSWORD", 22, 2);
        snprintf(line, sizeof(line), "Attempts left: %u", lock.attempts_left());
        centered(line, 50, 1);
        break;
    case LockLogic::LOCKOUT:
        centered("LOCKED OUT", 2, 2);
        snprintf(line, sizeof(line), "%lu", (unsigned long)lock.seconds_left(now));
        centered(line, 24, 3);
        centered("Too many attempts", 56, 1);
        break;
    }
    oled.display();
}

// ------------------------------------------------------------ setup / loop

void setup() {
    Serial.begin(115200);
    pinMode(RELAY_PIN, OUTPUT);
    pinMode(LED_GREEN_PIN, OUTPUT);
    pinMode(LED_RED_PIN, OUTPUT);
    buzzer.begin();
    setOutputs();

    Wire.begin(OLED_SDA, OLED_SCL);
    oled_ok = oled.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR);
    if (!oled_ok) Serial.println("SSD1306 not found, continuing without display");

    Serial.println("Door lock ready");
}

void handle(LockLogic::Event ev, uint32_t now) {
    switch (ev) {
    case LockLogic::EV_NONE:
        return;
    case LockLogic::EV_KEY:
    case LockLogic::EV_DELETED:
    case LockLogic::EV_CLEARED:
        buzzer.play(CLICK, 1);
        break;
    case LockLogic::EV_IGNORED:
        return;
    case LockLogic::EV_GRANTED:
        buzzer.play(SUCCESS, 5);
        Serial.println("ACCESS GRANTED: solenoid unlocked");
        break;
    case LockLogic::EV_DENIED:
        buzzer.play(FAILURE, 1);
        Serial.printf("WRONG PASSWORD: %u attempts left\n", lock.attempts_left());
        break;
    case LockLogic::EV_LOCKOUT:
        buzzer.play(ALARM, 7);
        Serial.printf("LOCKOUT: keypad disabled for %lu s\n", LOCKOUT_MS / 1000);
        break;
    case LockLogic::EV_RELOCKED:
        Serial.println("Relocked");
        break;
    case LockLogic::EV_LOCKOUT_END:
        Serial.println("Lockout over");
        break;
    case LockLogic::EV_READY:
        break;
    }
    setOutputs();
    redraw = true;
}

void loop() {
    static uint32_t last_scan = 0;
    uint32_t now = millis();
    buzzer.update(now);

    if (now - last_scan < SCAN_PERIOD_MS) return;
    last_scan = now;

    char k = keypad.getKey();
    if (k) {
        uint32_t t0 = micros();
        LockLogic::Event ev = lock.key(k, now);
        if (k == '#') Serial.printf("verify: %lu us\n", (unsigned long)(micros() - t0));
        handle(ev, now);
    }
    handle(lock.tick(now), now);

    // Refresh countdowns once a second
    if (lock.state() == LockLogic::UNLOCKED || lock.state() == LockLogic::LOCKOUT) {
        uint32_t s = lock.seconds_left(now);
        if (s != last_countdown) { last_countdown = s; redraw = true; }
    }
    if (redraw) {
        drawScreen(now);
        redraw = false;
    }
}

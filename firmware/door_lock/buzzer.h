// Non-blocking buzzer: plays a short list of notes while loop() keeps running.
#pragma once

#include <Arduino.h>

struct Note {
    uint16_t freq;   // Hz, 0 = rest
    uint16_t ms;
};

class Buzzer {
public:
    explicit Buzzer(uint8_t pin) : pin_(pin) {}

    void begin() { pinMode(pin_, OUTPUT); }

    void play(const Note *notes, uint8_t count) {
        notes_ = notes;
        count_ = count;
        index_ = 0;
        start(millis());
    }

    void update(uint32_t now) {
        if (index_ >= count_) return;
        if (now - started_ >= notes_[index_].ms) {
            ++index_;
            if (index_ < count_) {
                start(now);
            } else {
                noTone(pin_);
            }
        }
    }

private:
    void start(uint32_t now) {
        started_ = now;
        if (notes_[index_].freq) tone(pin_, notes_[index_].freq);
        else noTone(pin_);
    }

    uint8_t pin_;
    const Note *notes_ = nullptr;
    uint8_t count_ = 0, index_ = 0;
    uint32_t started_ = 0;
};

// Feedback patterns
static const Note CLICK[]   = {{2200, 30}};
static const Note SUCCESS[] = {{1047, 120}, {0, 40}, {1319, 120}, {0, 40}, {1568, 220}};
static const Note FAILURE[] = {{330, 400}};
static const Note ALARM[]   = {{880, 200}, {0, 80}, {880, 200}, {0, 80}, {880, 200}, {0, 80}, {880, 600}};

// Door-lock state machine. Plain C++ with no Arduino dependencies, so the
// same code runs on the ESP32 and in the host unit tests (test/).
//
//   ENTRY  --'#' correct-->  UNLOCKED  --UNLOCK_MS-->  ENTRY
//   ENTRY  --'#' wrong----->  WRONG (message) --MESSAGE_MS--> ENTRY
//   ENTRY  --'#' wrong, failed == max--> LOCKOUT --LOCKOUT_MS--> ENTRY
//
// Times are unsigned milliseconds; elapsed time is computed by unsigned
// subtraction, so millis() wrap-around after ~49.7 days is harmless.
#pragma once

#include <stdint.h>
#include <string.h>

class LockLogic {
public:
    enum State : uint8_t { ENTRY, UNLOCKED, WRONG, LOCKOUT };

    enum Event : uint8_t {
        EV_NONE,
        EV_KEY,          // digit accepted into the buffer
        EV_DELETED,      // last digit removed
        EV_CLEARED,      // buffer cleared
        EV_IGNORED,      // key not accepted (busy, buffer full, unused key)
        EV_GRANTED,      // correct password: unlock
        EV_DENIED,       // wrong password, attempts remain
        EV_LOCKOUT,      // wrong password, lockout started
        EV_RELOCKED,     // unlock window ended
        EV_LOCKOUT_END,  // lockout ended
        EV_READY,        // wrong-password message ended
    };

    LockLogic(const char *password, uint8_t max_attempts, uint32_t unlock_ms,
              uint32_t lockout_ms, uint32_t message_ms, uint8_t max_len)
        : password_(password), max_attempts_(max_attempts), unlock_ms_(unlock_ms),
          lockout_ms_(lockout_ms), message_ms_(message_ms),
          max_len_(max_len < kBufSize ? max_len : kBufSize - 1) {
        clear();
    }

    // Feed one key press. Returns what happened.
    Event key(char k, uint32_t now) {
        if (state_ != ENTRY) {
            return EV_IGNORED;
        }
        if (k >= '0' && k <= '9') {
            if (len_ >= max_len_) return EV_IGNORED;
            buf_[len_++] = k;
            buf_[len_] = '\0';
            return EV_KEY;
        }
        if (k == '*') {
            clear();
            return EV_CLEARED;
        }
        if (k == 'D') {
            if (len_ == 0) return EV_IGNORED;
            buf_[--len_] = '\0';
            return EV_DELETED;
        }
        if (k == '#') {
            return submit(now);
        }
        return EV_IGNORED;
    }

    // Call every loop iteration; handles the timed transitions.
    Event tick(uint32_t now) {
        uint32_t elapsed = now - since_;
        switch (state_) {
        case UNLOCKED:
            if (elapsed >= unlock_ms_) { state_ = ENTRY; return EV_RELOCKED; }
            break;
        case LOCKOUT:
            if (elapsed >= lockout_ms_) { state_ = ENTRY; failed_ = 0; return EV_LOCKOUT_END; }
            break;
        case WRONG:
            if (elapsed >= message_ms_) { state_ = ENTRY; return EV_READY; }
            break;
        default:
            break;
        }
        return EV_NONE;
    }

    State state() const { return state_; }
    bool relay_on() const { return state_ == UNLOCKED; }
    uint8_t entered() const { return len_; }
    uint8_t failed() const { return failed_; }
    uint8_t attempts_left() const { return max_attempts_ - failed_; }

    // Seconds left in the current timed state, rounded up (for countdowns).
    uint32_t seconds_left(uint32_t now) const {
        uint32_t total = state_ == UNLOCKED ? unlock_ms_ : state_ == LOCKOUT ? lockout_ms_ : 0;
        uint32_t elapsed = now - since_;
        if (total == 0 || elapsed >= total) return 0;
        return (total - elapsed + 999) / 1000;
    }

private:
    static const uint8_t kBufSize = 17;

    Event submit(uint32_t now) {
        bool ok = matches();
        clear();
        since_ = now;
        if (ok) {
            failed_ = 0;
            state_ = UNLOCKED;
            return EV_GRANTED;
        }
        if (++failed_ >= max_attempts_) {
            state_ = LOCKOUT;
            return EV_LOCKOUT;
        }
        state_ = WRONG;
        return EV_DENIED;
    }

    // Compares every character even after a mismatch, so the time taken
    // does not reveal how many leading digits were right.
    bool matches() const {
        size_t n = strlen(password_);
        uint8_t diff = (uint8_t)(n != len_);
        for (size_t i = 0; i < n; ++i) {
            char c = i < len_ ? buf_[i] : '\0';
            diff |= (uint8_t)(c ^ password_[i]);
        }
        return diff == 0;
    }

    void clear() {
        memset(buf_, 0, sizeof(buf_));
        len_ = 0;
    }

    const char *password_;
    uint8_t max_attempts_;
    uint32_t unlock_ms_, lockout_ms_, message_ms_;
    uint8_t max_len_;

    State state_ = ENTRY;
    uint32_t since_ = 0;
    uint8_t failed_ = 0;
    char buf_[kBufSize];
    uint8_t len_ = 0;
};

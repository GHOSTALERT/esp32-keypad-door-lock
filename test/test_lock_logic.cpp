// Host unit tests for the lock state machine (no Arduino needed).
//   make test
#include <cstdio>
#include <cstdlib>

#include "../firmware/door_lock/lock_logic.h"

static int failures = 0;
static int checks = 0;

#define CHECK(cond)                                                          \
    do {                                                                     \
        ++checks;                                                            \
        if (!(cond)) {                                                       \
            ++failures;                                                      \
            std::printf("  FAIL %s:%d  %s\n", __FILE__, __LINE__, #cond);    \
        }                                                                    \
    } while (0)

static const uint32_t UNLOCK = 5000, LOCKOUT = 30000, MESSAGE = 1500;

static LockLogic make() { return LockLogic("2580", 4, UNLOCK, LOCKOUT, MESSAGE, 8); }

static LockLogic::Event type(LockLogic &l, const char *keys, uint32_t now) {
    LockLogic::Event ev = LockLogic::EV_NONE;
    for (const char *p = keys; *p; ++p) ev = l.key(*p, now);
    return ev;
}

static void test_correct_password_unlocks_for_5s() {
    std::puts("correct password unlocks for 5 s");
    LockLogic l = make();
    CHECK(type(l, "2580#", 1000) == LockLogic::EV_GRANTED);
    CHECK(l.relay_on());
    CHECK(l.seconds_left(1000) == 5);
    CHECK(l.tick(5999) == LockLogic::EV_NONE);
    CHECK(l.relay_on());
    CHECK(l.tick(6000) == LockLogic::EV_RELOCKED);
    CHECK(!l.relay_on());
    CHECK(l.state() == LockLogic::ENTRY);
}

static void test_wrong_password_counts_down() {
    std::puts("wrong password shows attempts left");
    LockLogic l = make();
    CHECK(type(l, "1111#", 0) == LockLogic::EV_DENIED);
    CHECK(!l.relay_on());
    CHECK(l.attempts_left() == 3);
    CHECK(l.key('1', 100) == LockLogic::EV_IGNORED);   // message on screen
    CHECK(l.tick(1500) == LockLogic::EV_READY);
    CHECK(type(l, "2581#", 2000) == LockLogic::EV_DENIED);
    CHECK(l.attempts_left() == 2);
}

static void test_four_failures_lock_out_for_30s() {
    std::puts("four failures lock the keypad for 30 s");
    LockLogic l = make();
    uint32_t t = 0;
    for (int i = 0; i < 3; ++i) {
        CHECK(type(l, "0000#", t) == LockLogic::EV_DENIED);
        t += MESSAGE;
        CHECK(l.tick(t) == LockLogic::EV_READY);
    }
    CHECK(type(l, "0000#", t) == LockLogic::EV_LOCKOUT);
    CHECK(l.state() == LockLogic::LOCKOUT);
    CHECK(l.seconds_left(t) == 30);
    // Even the right password is ignored during lockout
    CHECK(type(l, "2580#", t + 10000) == LockLogic::EV_IGNORED);
    CHECK(!l.relay_on());
    CHECK(l.seconds_left(t + 10000) == 20);
    CHECK(l.tick(t + LOCKOUT - 1) == LockLogic::EV_NONE);
    CHECK(l.tick(t + LOCKOUT) == LockLogic::EV_LOCKOUT_END);
    CHECK(l.attempts_left() == 4);
    CHECK(type(l, "2580#", t + LOCKOUT + 10) == LockLogic::EV_GRANTED);
}

static void test_success_resets_failures() {
    std::puts("successful entry resets the failure count");
    LockLogic l = make();
    type(l, "9#", 0);
    l.tick(MESSAGE);
    type(l, "9#", 2000);
    l.tick(2000 + MESSAGE);
    CHECK(l.failed() == 2);
    CHECK(type(l, "2580#", 5000) == LockLogic::EV_GRANTED);
    CHECK(l.failed() == 0);
}

static void test_editing_keys() {
    std::puts("* clears, D deletes, A-C ignored, length capped");
    LockLogic l = make();
    CHECK(type(l, "99", 0) == LockLogic::EV_KEY);
    CHECK(l.key('*', 0) == LockLogic::EV_CLEARED);
    CHECK(l.entered() == 0);
    CHECK(l.key('D', 0) == LockLogic::EV_IGNORED);
    CHECK(type(l, "25809", 0) == LockLogic::EV_KEY);
    CHECK(l.key('D', 0) == LockLogic::EV_DELETED);
    CHECK(l.key('A', 0) == LockLogic::EV_IGNORED);
    CHECK(l.entered() == 4);
    CHECK(l.key('#', 0) == LockLogic::EV_GRANTED);

    LockLogic m = make();
    CHECK(type(m, "12345678", 0) == LockLogic::EV_KEY);
    CHECK(m.key('9', 0) == LockLogic::EV_IGNORED);
    CHECK(m.entered() == 8);
}

static void test_prefix_and_extension_rejected() {
    std::puts("prefix, extension and empty entry are rejected");
    LockLogic l = make();
    CHECK(type(l, "258#", 0) == LockLogic::EV_DENIED);
    l.tick(MESSAGE);
    CHECK(type(l, "25800#", 2000) == LockLogic::EV_DENIED);
    l.tick(2000 + MESSAGE);
    CHECK(type(l, "#", 4000) == LockLogic::EV_DENIED);
}

static void test_millis_wraparound() {
    std::puts("timers survive millis() wrap-around");
    LockLogic l = make();
    uint32_t t = 0xFFFFFFFFu - 2000;   // 2 s before wrap
    CHECK(type(l, "2580#", t) == LockLogic::EV_GRANTED);
    CHECK(l.tick(t + 4999) == LockLogic::EV_NONE);   // wrapped, 4.999 s later
    CHECK(l.tick(t + 5000) == LockLogic::EV_RELOCKED);
}

int main() {
    test_correct_password_unlocks_for_5s();
    test_wrong_password_counts_down();
    test_four_failures_lock_out_for_30s();
    test_success_resets_failures();
    test_editing_keys();
    test_prefix_and_extension_rejected();
    test_millis_wraparound();
    std::printf("%d checks, %d failures\n", checks, failures);
    return failures ? EXIT_FAILURE : EXIT_SUCCESS;
}

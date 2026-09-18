#include "silica_bees_native_wrapper.h"
#include <poll.h>
#include <time.h>

int64_t bees_clock_monotonic_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (int64_t)ts.tv_sec * 1000000000LL + (int64_t)ts.tv_nsec;
}

int64_t bees_clock_system_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return (int64_t)ts.tv_sec * 1000000000LL + (int64_t)ts.tv_nsec;
}

int64_t bees_poll_wait_ms(int64_t ms) {
    if (ms < 0) ms = 0;
    if (ms > 60000) ms = 60000;
    int64_t start = bees_clock_monotonic_ns();
    (void)poll((struct pollfd *)0, 0, (int)ms);
    return bees_clock_monotonic_ns() - start;
}

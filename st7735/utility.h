#include <time.h>


#define MIN(a, b)           ((a < b) ? a : b)
#define MAX(a, b)           ((a > b) ? a : b)
#define DIFF(a, b)          ((a > b) ? a - b : b - a)

#define SWAP_INT(a, b)      { int temp = a; a = b; b = temp; }
#define CLAMP(val, min, max) ((val < min) ? min : (val > max) ? max : val)

#define BCD_TO_DECIMAL(bcd)         ((bcd >> 4) * 10 + (bcd & 0x0F))            // Convert BCD to decimal
#define DECIMAL_TO_BCD(decimal)     (((decimal / 10) << 4) | (decimal % 10))    // Convert decimal to BCD

void print_hex(uint8_t *data, int len) {
    for (int i = 0; i < len; i++) {
        printf("%02X ", data[i]);
    }
    printf("\n");
}

static uint64_t elapse_ns(struct timespec *start) {
    struct timespec end;
    
    if (clock_gettime(CLOCK_MONOTONIC, &end) != 0) {
        perror("get_elapse_ms");
        return 0;
    }

    int64_t sec_diff = end.tv_sec - start->tv_sec;
    int64_t nsec_diff = end.tv_nsec - start->tv_nsec;
    
    // Handle negative nanoseconds (clock rollover)
    if (nsec_diff < 0) {
        sec_diff--;
        nsec_diff += 1E9;
    }
    
    // return nanoseconds
    return (sec_diff * 1E9) + nsec_diff;
}

uint64_t get_elapse_nanoSec(void callback()) {
    struct timespec start, end;
    
    //! Start timer
    if (clock_gettime(CLOCK_MONOTONIC, &start) != 0) {
        perror("clock_gettime");
        return 0;
    }
    
    //! Execute callback
    callback();
    
    return elapse_ns(&start);
}

void print_elapse_nanoSec(const char *str, void callback(), int print_log) {
    uint64_t elapse = get_elapse_nanoSec(callback);
    if (print_log) printf("%s:\t\t %llu ns\n", str, elapse);
}

void print_elapse_microSec(const char *str, void callback(), int print_log) {
    uint64_t elapse = get_elapse_nanoSec(callback) / 1000;
    if (print_log) printf("%s:\t\t %llu us\n", str, elapse);
}

uint64_t get_elapse_microSec(void callback()) {
    return get_elapse_nanoSec(callback) / 1000;
}

uint64_t millis() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000 + ts.tv_nsec / 1E6;
}

uint32_t micros() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1E6 + ts.tv_nsec / 1E3;
}

uint32_t seconds() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec;
}

int refresh_change_time(uint32_t *last_change_time) {
    uint32_t now = millis();
    if (now - *last_change_time < 200) return -1;
    *last_change_time = now;
    return now;
}
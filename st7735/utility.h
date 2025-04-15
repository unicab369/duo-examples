#include <time.h>


#define MIN(a, b)           ((a < b) ? a : b)
#define MAX(a, b)           ((a > b) ? a : b)
#define DIFF(a, b)          ((a > b) ? a - b : b - a)

#define SWAP_INT(a, b)    { int temp = a; a = b; b = temp; }
#define CLAMP(val, min, max) ((val < min) ? min : (val > max) ? max : val)

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
    if (print_log) printf("%s:\t %llu ns\n", str, elapse);
}

void print_elapse_microSec(const char *str, void callback(), int print_log) {
    uint64_t elapse = get_elapse_nanoSec(callback) / 1000;
    if (print_log) printf("%s:\t %llu us\n", str, elapse);
}

uint64_t get_elapse_microSec(void callback()) {
    return get_elapse_nanoSec(callback) / 1000;
}


static struct timespec ref_timer;

void start_timer() {    
    if (clock_gettime(CLOCK_MONOTONIC, &ref_timer) != 0) {
        perror("clock_gettime");
        return;
    }
}

uint64_t stop_timer() {
    return elapse_ns(&ref_timer);
}

void print_hex(uint8_t *data, int len) {
    for (int i = 0; i < len; i++) {
        printf("%02X ", data[i]);
    }
    printf("\n");
}
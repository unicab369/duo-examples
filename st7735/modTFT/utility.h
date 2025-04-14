#include <time.h>


#define MIN(a, b)           ((a < b) ? a : b)
#define MAX(a, b)           ((a > b) ? a : b)
#define DIFF(a, b)          ((a > b) ? a - b : b - a)
#define SWAP_INT(a, b)    { int temp = a; a = b; b = temp; }

static uint64_t elapse_ms(struct timespec *start) {
    struct timespec end;
    
    if (clock_gettime(CLOCK_MONOTONIC, &end) != 0) {
        perror("get_elapse_ms");
        return 0;
    }

    // Calculate microseconds with proper handling of negative nanoseconds
    int64_t sec_diff = end.tv_sec - start->tv_sec;
    int64_t nsec_diff = end.tv_nsec - start->tv_nsec;
    
    // Handle negative nanoseconds (clock rollover)
    if (nsec_diff < 0) {
        sec_diff--;
        nsec_diff += 1000000000;
    }
    
    // Convert to microseconds
    return (sec_diff * 1000000) + (nsec_diff / 1000);
}

uint64_t get_elapse_time_ms(void callback()) {
    struct timespec start, end;
    
    //! Start timer
    if (clock_gettime(CLOCK_MONOTONIC, &start) != 0) {
        perror("clock_gettime");
        return 0;
    }
    
    //! Execute callback
    callback();
    
    return elapse_ms(&start);
}


static struct timespec start_timer;

void start_elapse_timer() {    
    if (clock_gettime(CLOCK_MONOTONIC, &start_timer) != 0) {
        perror("clock_gettime");
        return;
    }
}

uint64_t stop_elapse_timer() {
    return elapse_ms(&start_timer);
}

void print_hex(uint8_t *data, int len) {
    for (int i = 0; i < len; i++) {
        printf("%02X ", data[i]);
    }
    printf("\n");
}
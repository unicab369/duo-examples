#include "modSSD1306_line.h"

int modI2C_init() {
    if ((fd_i2c = wiringXI2CSetup("/dev/i2c-1", 0x3C)) <0) {
        printf("I2C Setup failed: %i\n", fd_i2c);
        return -1;
    }

    ssd1306_init();

    // ssd1306_push_string(0, 0, "Hello MilkV Duozzz!", 8);
    // ssd1306_push_string(0, 1, "Hello MilkV Duozzz!", 16);

    return 1;
}

void test_clearScreen1() {
    modSSD1306_clearScreen(0xFF);
}

void test_horLine() {
    prefill_horLine(10, 0, SSD1306_W, 3, 0);
}

void test_verLine() {
    prefill_verLine(10, 0, SSD1306_H, 3, 0);
}

void test_bLine() {
    prefill_line(0, 0, SSD1306_H, SSD1306_H, 3);
}

void modI2C_task(int print_log) {
    uint64_t elapse;

    // elapse = get_elapse_microSec(test_clearScreen1);
    // if (print_log) printf("test_clearScreen1:\t %llu us\n", elapse);

    elapse = get_elapse_nanoSec(test_horLine);
    if (print_log) printf("test_horLine:\t\t %llu ns\n", elapse);

    elapse = get_elapse_nanoSec(test_verLine);
    if (print_log) printf("test_verLine:\t\t %llu ns\n", elapse);

    elapse = get_elapse_nanoSec(test_bLine);
    if (print_log) printf("test_bLine:\t\t %llu ns\n", elapse);

    elapse = get_elapse_microSec(ssd1306_renderFrame);
    if (print_log) printf("ssd1306_renderFrame:\t %llu us\n", elapse);

    delayMicroseconds(400E3);
    modSSD1306_clearScreen(0x00);

    printf("\n");
}
#include "modSSD1306_draw.h"

int modI2C_init() {
    if ((fd_i2c = wiringXI2CSetup("/dev/i2c-1", 0x3C)) <0) {
        printf("I2C Setup failed: %i\n", fd_i2c);
        return -1;
    }

    ssd1306_init();
    return 1;
}


void modI2C_task(int print_log) {
    test_prefillLines(print_log);
}
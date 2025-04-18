
#include "st7735.h"
#include "modI2C/modI2C.h"

#include <sys/reboot.h>

#include "modGPIO/modEncoder.h"
#include "modGPIO/modJoystick.h"

void reboot_system() {
    printf("\nRebooting system...\n");
    reboot(RB_AUTOBOOT);
}

void encoder_cb(int position, int direction) {
    printf("Position: %d, Direction: %d\n", position, direction);
}

void joystick_cb(int x, int y) {
    printf("x: %d, y: %d\n", x, y);
}

int main() {
    st7735_init();
    modI2C_init();

    M_SSD1306 ssd1306_0 = {
        .channel = 0,
        .address = 0x3C,
        .print_log = 0
    };
    modI2C_intSSD1306(&ssd1306_0);

    M_SSD1306 ssd1306_1 = {
        .channel = 1,
        .address = 0x3C,
        .print_log = 0
    };
    modI2C_intSSD1306(&ssd1306_1);

    M_Encoder encoder = {
        .DT_PIN = 21,
        .CLK_PIN = 20,
    };
    encoder_init(&encoder);
    
    M_Joystick joystick = {
        .X_PIN = 26,
        .Y_PIN = 27,
    };
    joystick_init(&joystick);

    while(1) {
        st7735_task(0);
        modI2C_task(0);
        encoder_task(&encoder, encoder_cb);
        joystick_task(&joystick, joystick_cb);

        test_ssd1306_draw(&ssd1306_0);
        test_ssd1306_draw(&ssd1306_1);
    }

    return 0;
}
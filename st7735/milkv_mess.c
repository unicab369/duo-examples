
#include "st7735.h"
#include "modI2C/modI2C.h"

#include <sys/reboot.h>

#include "modGPIO/modEncoder.h"
#include "modGPIO/modJoystick.h"
#include "modGPIO/modPIR.h"
#include "modGPIO/modLED.h"

void reboot_system() {
    printf("\nRebooting system...\n");
    reboot(RB_AUTOBOOT);
}

M_SSD1306 ssd1306_0 = {
    .channel = 0,
    .address = 0x3C,
    .print_log = 0
};

M_SSD1306 ssd1306_1 = {
    .channel = 1,
    .address = 0x3C,
    .print_log = 0
};

M_LED led = {
    .LED_PIN = 18,
};

void encoder_cb(int position, int direction) {
    // printf("Position: %d, Direction: %d\n", position, direction);

    char output[32];
    snprintf(output, sizeof(output), "pos: %d, dir: %d", position, direction);
    compute_str_atLine(6, 0, output);

    ssd1306_setHost(ssd1306_0.host);
    ssd1306_renderFrame();

    ssd1306_setHost(ssd1306_1.host);
    ssd1306_renderFrame();
}

void joystick_cb(int x, int y) {
    // printf("x: %d, y: %d\n", x, y);

    char output[32];
    snprintf(output, sizeof(output), "x: %d, y: %d", x, y);
    compute_str_atLine(7, 0, output);

    ssd1306_setHost(ssd1306_0.host);
    ssd1306_renderFrame();

    ssd1306_setHost(ssd1306_1.host);
    ssd1306_renderFrame();
}

void pir_cb(int state) {
    printf("PIR: %d\n", state);
    led_set(&led, state);
}

#include "modWS2812/modWS2812.h"

int main() {
    test_led();

    st7735_init();
    modI2C_init();

    modI2C_intSSD1306(&ssd1306_0);
    modI2C_intSSD1306(&ssd1306_1);

    M_Encoder encoder = {
        .DT_PIN = 21,
        .CLK_PIN = 20,
        .print_log = 0
    };
    encoder_init(&encoder);
    
    M_Joystick joystick = {
        .X_PIN = 26,
        .Y_PIN = 27,
        .print_log = 0
    };
    joystick_init(&joystick);


    M_PIR pir = {
        .PIR_PIN = 17,
        .print_log = 0
    };
    pir_init(&pir);

    led_init(&led);

    while(1) {
        // modI2C_task(0);
        encoder_task(&encoder, encoder_cb);
        joystick_task(&joystick, joystick_cb);
        pir_task(&pir, pir_cb);

        st7735_task(0);
        // test_ssd1306_draw(&ssd1306_0);
        // test_ssd1306_draw(&ssd1306_1);
    }

    return 0;
}
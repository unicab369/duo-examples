
#include "st7735.h"
#include "modI2C/modI2C.h"

#include <sys/reboot.h>
#include "modGPIO/modEncoder.h"

void reboot_system() {
    printf("\nRebooting system...\n");
    reboot(RB_AUTOBOOT);
}

void encoder_cb(int position, int direction) {
    printf("Position: %d, Direction: %d\n", position, direction);
}

M_Encoder encoder = {
    .DT_PIN = 20,
    .CLK_PIN = 21,
    .callback = encoder_cb
};  

int main() {
    st7735_init();
    modI2C_init();

    encoder_init(&encoder);

    pinMode(26, PINMODE_OUTPUT);
    pinMode(27, PINMODE_OUTPUT);

    digitalWrite(26, HIGH);
    digitalWrite(27, HIGH);

    while(1) {
        // st7735_task(0);
        // modI2C_task(1);
        encoder_task(&encoder);
    }

    return 0;
}
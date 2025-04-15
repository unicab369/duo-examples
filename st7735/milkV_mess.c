
#include "st7735.h"
#include "modI2C/modI2C.h"

#include <sys/reboot.h>

void reboot_system() {
    printf("\nRebooting system...\n");
    reboot(RB_AUTOBOOT);
}

int main() {
    st7735_init();
    modI2C_init();

    while(1) {
        st7735_task(0);
        modI2C_task(1);
    }

    return 0;
}
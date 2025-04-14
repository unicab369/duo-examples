
#include "st7735.h"
#include "modI2C/modI2C.h"

int main() {
    st7735_init();
    modI2C_init();

    printf("IM HEREzzz\n");

    while(1) {
        st7735_task(0);
        modI2C_task(1);
    }

    return 0;
}
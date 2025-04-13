#include <stdio.h>
#include <unistd.h>
#include <stdint.h>

#include <wiringx.h>

#include "font_bitmap.h"
#include "st7735_shape.h"

#define SPI_NUM     2
#define SPI_SPEED   20E6

M_Spi_Conf conf = {
    .HOST = 2,
    .CS = 15,      //! the main cs pin
    .DC = 5,       // set to -1 when not use
    .RST = 4       // set to -1 when not use
};

void fill_screen() {
    modTFT_fillScreen(BLUE, &conf);
}

int main() {
    if(wiringXSetup("milkv_duo", NULL) == -1) {
        wiringXGC();
        return 1;
    }

    if (wiringXSPISetup(SPI_NUM, SPI_SPEED) < 0) {
        printf("SPI Setup failed.\n");
        return -1;
    }

    if (conf.CS > 0) pinMode(conf.CS, PINMODE_OUTPUT);
    if (conf.DC > 0) pinMode(conf.DC, PINMODE_OUTPUT);

    if (conf.RST > 0) {
        pinMode(conf.RST, PINMODE_OUTPUT);
        digitalWrite(conf.RST, HIGH);

        digitalWrite(conf.RST, LOW);
        delayMicroseconds(10);
        digitalWrite(conf.RST, HIGH);
    }


    modTFT_init(&conf);

    M_TFT_Text tft_text = {
        .x = 0, .y = 0,
        .color = PURPLE,
        .page_wrap = 1,
        .word_wrap = 1,

        .font = (const uint8_t *)FONT_7x5,      // Pointer to the font data
        .font_width = 5,                        // Font width
        .font_height = 7,                       // Font height
        .char_spacing = 1,                      // Spacing between characters
        .text = "What is Thy bidding, my master? Tell me!"
                "\nTomorrow is another day!"
                "\n\nThis is a new line. Continue with this line."
    };

    st7735_draw_horLine(80, 10, 100, PURPLE, 3, &conf);
    st7735_draw_verLine(80, 10, 100, PURPLE, 3, &conf);
    st7735_draw_rectangle(20, 20, 50, 50, PURPLE, 3, &conf);

    while(1) {
        st7735_draw_horLine(80, 10, 100, PURPLE, 3, &conf);
        st7735_draw_verLine(80, 10, 100, PURPLE, 3, &conf);
        digitalWrite(conf.CS, HIGH);
        sleep(1);

        uint64_t elapse = get_elapse_time_ms(fill_screen);
        printf("Elapse time: %llu us\n", elapse);        

        digitalWrite(conf.CS, LOW);
        sleep(1);
    }

    return 0;
}

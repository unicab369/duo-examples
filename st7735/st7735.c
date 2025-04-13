#include <stdio.h>
#include <unistd.h>
#include <stdint.h>

#include <wiringx.h>

#include "font_bitmap.h"
#include "st7735_shape.h"

#define SPI_NUM     2
#define SPI_SPEED   50E6

M_Spi_Conf conf = {
    .HOST = 2,
    .CS = 15,      //! the main cs pin
    .DC = 5,       // set to -1 when not use
    .RST = 4       // set to -1 when not use
};

#define LINE_COLLOR RED

void fill_screen() {
    modTFT_fillRect(BLUE, ST7735_WIDTH, ST7735_HEIGHT, &conf);
}

void draw_horLine() {
    st7735_draw_horLine(80, 10, 100, LINE_COLLOR, 3, &conf);
}

void draw_verLine() {
    st7735_draw_verLine(80, 10, 100, LINE_COLLOR, 3, &conf);
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

    modTFT_init(&conf);

    M_TFT_Text tft_text = {
        .x = 0, .y = 0,
        .color = RED,
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


    while(1) {
        uint64_t elapse;
        
        // elapse = get_elapse_time_ms(fill_screen);
        // printf("fill_rect time: %llu us\n", elapse);       

        elapse = get_elapse_time_ms(draw_horLine);
        printf("hor_line time: %llu us\n", elapse);       

        elapse = get_elapse_time_ms(draw_verLine);
        printf("ver_line timez: %llu us\n", elapse);

        printf("zzzzzzzzzzzzzz\n");
        digitalWrite(conf.CS, HIGH);
        delayMicroseconds(400E3);

        digitalWrite(conf.CS, LOW);
        delayMicroseconds(400E3);
    }

    return 0;
}

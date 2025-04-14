#include <stdio.h>
#include <unistd.h>
#include <stdint.h>

#include <wiringx.h>

#include "font_bitmap.h"
#include "modTFT_line.h"

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
    modTFT_fillAll(BLUE, &conf);
}

void draw_horLine() {
    modTFT_draw_horLine(80, 10, 100, LINE_COLLOR, 3, &conf);
}

void draw_verLine() {
    modTFT_draw_verLine(80, 10, 100, LINE_COLLOR, 3, &conf);
}

void draw_line1() {
    draw_line_bresenham(1, 1, 100, 100, WHITE, 5, &conf);
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
    modTFT_fillAll(RED, &conf); // Fill screen with black

    M_TFT_Text tft_text = {
        .x = 0, .y = 0,
        .color = WHITE,
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

        // elapse = get_elapse_time_ms(draw_horLine);
        // printf("hor_line time: %llu us\n", elapse);       

        // elapse = get_elapse_time_ms(draw_verLine);
        // printf("ver_line time: %llu us\n", elapse);

        // elapse = get_elapse_time_ms(test_draw2);
        // printf("test_draw2 time: %llu us\n", elapse);

        // elapse = get_elapse_time_ms(test_draw1);
        // printf("test_draw1 time: %llu us\n", elapse);

        elapse = get_elapse_time_ms(draw_line1);
        printf("draw_line1 time: %llu us\n", elapse);

        printf("zzzz\n");
        digitalWrite(conf.CS, HIGH);
        delayMicroseconds(400E3);

        digitalWrite(conf.CS, LOW);
        delayMicroseconds(400E3);
    }

    return 0;
}

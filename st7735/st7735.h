#include <stdio.h>
#include <unistd.h>
#include <stdint.h>

#include <wiringx.h>

#include "FONT_BITMAP.h"
#include "modTFT/modTFT_shape.h"
#include "modTFT/modTFT_text.h"


#define SPI_NUM     2
#define SPI_SPEED   50E6

M_Spi_Conf conf = {
    .HOST = 2,
    .CS = 9,      //! the main cs pin
    .DC = 4,       // set to -1 when not use
    .RST = 5       // set to -1 when not use
    // .DC = 5,       // set to -1 when not use
    // .RST = 4       // set to -1 when not use
};

#define LINE_COLLOR ST7735_ORANGE

void fill_screen() {
    modTFT_fillAll(ST7735_BLUE, &conf);
}

void draw_horLine() {
    modTFT_drawHorLine(80, 10, 100, LINE_COLLOR, 3, &conf);
}

void draw_verLine() {
    modTFT_drawVerLine(80, 10, 100, LINE_COLLOR, 3, &conf);
}

void draw_line1() {
    modTFT_drawLine(1, 1, 100, 100, LINE_COLLOR, 5, &conf);
}

void fill_rect() {
    modTFT_fillRect(0, 0, 20, 20, ST7735_BLUE, &conf);
}

void draw_circle() {
    modTFT_drawCircle(30, 30, 15, LINE_COLLOR, &conf);
}

void fill_circle() {
    modTFT_drawFilledCircle(50, 50, 15, ST7735_BLACK, &conf);
}

void draw_rectangle() {
    modTFT_drawRect(70, 70, 90, 90, LINE_COLLOR, 2, &conf);
}

void draw_poly() {
    int x_points[] = {90, 95, 110, 100, 105, 90, 75, 80, 70, 85}; 
    int y_points[] = {90, 115, 115, 130, 155, 140, 155, 130, 115, 115};
    
    // Draw thick blue triangle
    modTFT_drawPoly(
        x_points, y_points, 10, ST7735_WHITE, 2, &conf
    );
}

void draw_filled_poly() {
    int x_points[] = {20, 50, 80};
    int y_points[] = {100, 80, 100};
    
    // Draw filled blue triangle
    modTFT_drawFilledPoly(
        x_points, y_points, 3, ST7735_BLUE, &conf
    );
}

int st7735_init() {
    if(wiringXSetup("milkv_duo", NULL) == -1) {
        wiringXGC();
        return 1;
    }

    if (wiringXSPISetup(SPI_NUM, SPI_SPEED) < 0) {
        printf("SPI Setup failed.\n");
        return -1;
    }

    modTFT_init(&conf);
    modTFT_fillAll(ST7735_RED, &conf); // Fill screen with black

    M_TFT_Text tft_text = {
        .x = 0, .y = 0,
        .color = ST7735_YELLOW,
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

    modTFT_drawText(&tft_text, &conf);

    return 1;
}

static uint32_t last_st7735_time = 0;

void st7735_task(int print_log) {
    uint32_t now = millis();
    if (now - last_st7735_time < 100) {return; }
    last_st7735_time = now;
    
    uint64_t elapse;
        
    digitalWrite(conf.CS, LOW);

    // elapse = get_elapse_microSec(fill_screen);
    // printf("fill_rect time: %llu us\n", elapse);       

    elapse = get_elapse_microSec(draw_horLine);
    if (print_log) printf("hor_line:\t\t %llu us\n", elapse);       

    elapse = get_elapse_microSec(draw_verLine);
    if (print_log) printf("ver_line:\t\t %llu us\n", elapse);

    elapse = get_elapse_microSec(draw_line1);
    if (print_log) printf("draw_line1:\t\t %llu us\n", elapse);

    elapse = get_elapse_microSec(fill_rect);
    if (print_log) printf("fill_rect:\t\t %llu us\n", elapse);

    elapse = get_elapse_microSec(draw_circle);
    if (print_log) printf("draw_circle:\t\t %llu us\n", elapse);

    elapse = get_elapse_microSec(fill_circle);
    if (print_log) printf("fill_circle:\t\t %llu us\n", elapse);

    elapse = get_elapse_microSec(draw_rectangle);
    if (print_log) printf("draw_rect:\t\t %llu us\n", elapse);

    elapse = get_elapse_microSec(draw_poly);
    if (print_log) printf("draw_poly:\t\t %llu us\n", elapse);

    elapse = get_elapse_microSec(draw_filled_poly);
    if (print_log) printf("draw_filled_poly:\t %llu us\n", elapse);

    if (print_log) printf("\n");
    digitalWrite(conf.CS, HIGH);

    // delayMicroseconds(400E3);
}
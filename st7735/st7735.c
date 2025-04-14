#include <stdio.h>
#include <unistd.h>
#include <stdint.h>

#include <wiringx.h>

#include "font_bitmap.h"
#include "modTFT/modTFT_shape.h"

#define SPI_NUM     2
#define SPI_SPEED   50E6

M_Spi_Conf conf = {
    .HOST = 2,
    .CS = 15,      //! the main cs pin
    .DC = 5,       // set to -1 when not use
    .RST = 4       // set to -1 when not use
};

#define LINE_COLLOR ORANGE

void fill_screen() {
    modTFT_fillAll(BLUE, &conf);
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
    modTFT_fillRect(0, 0, 20, 20, BLUE, &conf);
}

void draw_circle() {
    modTFT_drawCircle(30, 30, 15, LINE_COLLOR, &conf);
}

void fill_circle() {
    modTFT_drawFilledCircle(50, 50, 15, BLACK, &conf);
}

void draw_rectangle() {
    modTFT_drawRect(70, 70, 90, 90, LINE_COLLOR, 2, &conf);
}

void draw_poly() {
    int x_points[] = {90, 95, 110, 100, 105, 90, 75, 80, 70, 85}; 
    int y_points[] = {90, 115, 115, 130, 155, 140, 155, 130, 115, 115};
    
    // Draw thick blue triangle
    modTFT_drawPoly(
        x_points, y_points, 10, WHITE, 2, &conf
    );
}

void draw_filled_poly() {
    int x_points[] = {20, 50, 80};
    int y_points[] = {100, 80, 100};
    
    // Draw filled blue triangle
    modTFT_drawFilledPoly(
        x_points, y_points, 3, BLUE, &conf
    );
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
        .color = ORANGE,
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
        printf("hor_line:\t\t %llu us\n", elapse);       

        elapse = get_elapse_time_ms(draw_verLine);
        printf("ver_line:\t\t %llu us\n", elapse);

        elapse = get_elapse_time_ms(draw_line1);
        printf("draw_line1:\t\t %llu us\n", elapse);

        elapse = get_elapse_time_ms(fill_rect);
        printf("fill_rect:\t\t %llu us\n", elapse);

        elapse = get_elapse_time_ms(draw_circle);
        printf("draw_circle:\t\t %llu us\n", elapse);

        elapse = get_elapse_time_ms(fill_circle);
        printf("fill_circle:\t\t %llu us\n", elapse);

        elapse = get_elapse_time_ms(draw_rectangle);
        printf("draw_rect:\t\t %llu us\n", elapse);

        elapse = get_elapse_time_ms(draw_poly);
        printf("draw_poly:\t\t %llu us\n", elapse);

        elapse = get_elapse_time_ms(draw_filled_poly);
        printf("draw_filled_poly:\t %llu us\n", elapse);

        printf("\n");
        digitalWrite(conf.CS, HIGH);
        delayMicroseconds(400E3);

        digitalWrite(conf.CS, LOW);
        delayMicroseconds(400E3);
    }

    return 0;
}

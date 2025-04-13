#include <stdio.h>
#include <unistd.h>
#include <stdint.h>  

#include "font_bitmap.h"

// #include "mod_st7735.h"
#include "st7735_shape.h"


int main() {
    if(wiringXSetup("milkv_duo", NULL) == -1) {
        wiringXGC();
        return 1;
    }

    M_Spi_Conf conf = {
        .host = 2,
        .cs = 15,      //! the main cs pin
        .dc = 5,       // set to -1 when not use
        .rst = 4       // set to -1 when not use
    };

    mod_spi_init(&conf, 10E6);
    st7735_init(&conf);

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
        // st7735_draw_horLine(80, 10, 100, PURPLE, 3, &conf);
        // st7735_draw_verLine(80, 10, 100, PURPLE, 3, &conf);

        // st7735_fill_screen(RED, &conf);
        digitalWrite(CS_PIN, HIGH);
        sleep(1);

        // st7735_fill_screen(BLUE, &conf);
        digitalWrite(CS_PIN, LOW);
        sleep(1);

        printf("IM HERE 2222\n");
    }

    return 0;
}

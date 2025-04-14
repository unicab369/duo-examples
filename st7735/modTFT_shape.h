
#include <stdio.h>
#include <unistd.h>
#include <stdint.h>  
#include <string.h>
#include <stdlib.h>

#include "modTFT_line.h"


//# Draw Rectangle
void st7735_draw_rectangle(
    uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, 
    uint16_t color, uint8_t thickness, M_Spi_Conf *config
) {
    modTFT_draw_horLine(y0, x0, x1, color, thickness, config);
    modTFT_draw_horLine(y1, x0, x1, color, thickness, config);
    modTFT_draw_verLine(x0, y0, y1, color, thickness, config);
    modTFT_draw_verLine(x1, y0, y1, color, thickness, config);
}
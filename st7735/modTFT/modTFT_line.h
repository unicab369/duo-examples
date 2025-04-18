
#include <stdio.h>
#include <unistd.h>
#include <stdint.h>  
#include <string.h>
#include <stdlib.h>

#include "modTFT.h"

#define OUTPUT_BUFFER_SIZE 100  // Number of pixels to buffer before sending
#define MAX_CHUNK_PIXELS 1024


uint16_t color_buffer[OUTPUT_BUFFER_SIZE];

//# Draw horizontal line
void modTFT_drawHorLine(
    int y, int x0, int x1, 
    uint16_t color, int thickness, M_Spi_Conf *config
) {
    if (x0 > x1) SWAP_INT(x0, x1);

    static uint32_t chunk_buffer[(MAX_CHUNK_PIXELS + 1) / 2];

    // Pack 2 pixels per word
    uint32_t color32 = (color << 16) | color;
    for (int i = 0; i < sizeof(chunk_buffer)/sizeof(chunk_buffer[0]); i++) {
        chunk_buffer[i] = color32;  // Compiler may optimize this
    }

    //! Set CS
    digitalWrite(config->CS, 0);

    // Draw chunks
    int chunk_bytes;
    for (int x = x0; x <= x1; x += MAX_CHUNK_PIXELS) {
        int width = MIN(MAX_CHUNK_PIXELS, x1 - x + 1);
        chunk_bytes = width * thickness * 2;

        //# Set window & send data
        modTFT_setWindow(x, y, x + width - 1, y + thickness - 1, config);
        spi_send_data((uint8_t*)chunk_buffer, chunk_bytes, config);
    }

    //! Set CS
    digitalWrite(config->CS, 1);
}

//# Draw vertical line
void modTFT_drawVerLine(
    int x, int y0, int y1,
    uint16_t color, int thickness, M_Spi_Conf *config
) {
    if (y0 > y1) SWAP_INT(y0, y1);

    static uint32_t chunk_buffer[(MAX_CHUNK_PIXELS + 1) / 2];

    // Pack 2 pixels per word
    uint32_t color32 = (color << 16) | color;
    for (int i = 0; i < sizeof(chunk_buffer)/sizeof(chunk_buffer[0]); i++) {
        chunk_buffer[i] = color32;  // Compiler may optimize this
    }

    //! Set CS
    digitalWrite(config->CS, 0);

    // Draw chunks
    int chunk_bytes;
    for (int y = y0; y <= y1; y += MAX_CHUNK_PIXELS) {
        int height = MIN(MAX_CHUNK_PIXELS, y1 - y + 1);
        chunk_bytes = height * thickness * 2;
        
        //# Set window & send data
        modTFT_setWindow(x, y, x + thickness - 1, y + height - 1, config);
        spi_send_data((uint8_t*)chunk_buffer, chunk_bytes, config);
    }

    //! Set CS
    digitalWrite(config->CS, 1);
}

//# draw_line_bresenham
static void draw_line_bresenham_slow(
    int x0, int y0, int x1, int y1,
    uint16_t color, int width, M_Spi_Conf *config
) {
    uint8_t steep = DIFF(y1, y0) > DIFF(x1, x0);
    if (steep) {
        SWAP_INT(x0, y0);
        SWAP_INT(x1, y1);
    }

    if (x0 > x1) {
        SWAP_INT(x0, x1);
        SWAP_INT(y0, y1);
    }

    int dx   = x1 - x0;
    int dy   = DIFF(y1, y0);
    int err  = dx >> 1;
    int step = (y0 < y1) ? 1 : -1;

    //! Set CS
    digitalWrite(config->CS, 0);

    for (; x0 <= x1; x0++) {
        for (int w = -(width / 2); w <= width / 2; w++) {
            if (steep) {
                modTFT_drawPixel(y0 + w, x0, color, config); // Draw perpendicular pixels for width
            } else {
                modTFT_drawPixel(x0, y0 + w, color, config); // Draw perpendicular pixels for width
            }
        }
        
        err -= dy;
        if (err < 0) {
            err += dx;
            y0 += step;
        }
    }

    //! Set CS
    digitalWrite(config->CS, 1);
}

void draw_line_bresenham(
    int x0, int y0, int x1, int y1,
    uint16_t color, int thickness, M_Spi_Conf *config
) {
    // Determine steepness and sort coordinates
    int steep = abs(y1 - y0) > abs(x1 - x0);
    if (steep) {
        SWAP_INT(x0, y0);
        SWAP_INT(x1, y1);
    }
    if (x0 > x1) {
        SWAP_INT(x0, x1);
        SWAP_INT(y0, y1);
    }

    // Precompute all invariants
    const int half_width = thickness >> 1;
    const int width_start = -half_width;
    const int width_end = half_width + (thickness & 1);

    const int dy = abs(y1 - y0);
    const int dx = x1 - x0;
    const int ystep = (y0 < y1) ? 1 : -1;
    int err = dx >> 1;

    //! Set CS
    digitalWrite(config->CS, 0);

    // Precompute the SPI-ready color (bytes swapped for ST7735)
    const uint16_t spi_color = ((color & 0xFF) << 8) | (color >> 8);

    // Prepare pixel buffer for bulk writes
    uint16_t pixel_buffer[16]; // Sized for common widths
    for (int i = 0; i < sizeof(pixel_buffer)/sizeof(pixel_buffer[0]); i++) {
        pixel_buffer[i] = color;
    }

    // Main drawing loop
    while (x0 <= x1) {
        // Calculate perpendicular coordinates
        if (steep) {
            const int base_y = y0;
            for (int w = width_start; w <= width_end; w++) {
                modTFT_drawPixel(base_y + w, x0, color, config);
            }
        } else {
            const int base_x = x0;
            if (thickness == 1) {
                // Fast path for single-pixel width
                modTFT_drawPixel(base_x, y0, color, config);
            } else {
                // Bulk write optimization for common widths
                if (thickness <= 16) {
                    modTFT_setWindow(base_x, y0 + width_start, base_x, y0 + width_end, config);
                    spi_send_data((uint8_t*)pixel_buffer, thickness * 2, config);
                } else {
                    // Fallback for large widths
                    for (int w = width_start; w <= width_end; w++) {
                        modTFT_drawPixel(base_x, y0 + w, color, config);
                    }
                }
            }
        }

        // Bresenham error calculation
        err -= dy;
        if (err < 0) {
            err += dx;
            y0 += ystep;
        }
        x0++;
    }

    //! Set CS
    digitalWrite(config->CS, 1);
}

void modTFT_drawLine(
    int x0, int y0, int x1, int y1,
    uint16_t color, int thickness, M_Spi_Conf *config
) {
    //! Handle horizontal lines (optimized path)
    if (y0 == y1) {
        modTFT_drawHorLine(y0, x0, x1, color, thickness, config);
        return;
    }
    
    //! Handle vertical lines (optimized path)
    if (x0 == x1) {
        modTFT_drawVerLine(x0, y0, y1, color, thickness, config);
        return;
    }

    //! Handle diagonal lines (Bresenham's algorithm)
    draw_line_bresenham(x0, y0, x1, y1, color, thickness, config);
}
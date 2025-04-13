#include <stdio.h>
#include <unistd.h>
#include <stdint.h>  
#include <string.h>
#include <stdlib.h>

#include "modTFT.h"

#define OUTPUT_BUFFER_SIZE 100  // Number of pixels to buffer before sending
#define MAX_CHUNK_PIXELS 1024


uint16_t color_buffer[OUTPUT_BUFFER_SIZE];

//# Draw line
void st7735_draw_line(int16_t x0, int16_t y0, int16_t x1, int16_t y1,
                     uint16_t color, M_Spi_Conf *config) {
    // Bresenham's line algorithm
    int dx = abs(x1 - x0);
    int dy = abs(y1 - y0);
    int sx = (x0 < x1) ? 1 : -1;
    int sy = (y0 < y1) ? 1 : -1;
    int err = (dx > dy ? dx : -dy) / 2;
    int e2;

    uint16_t buffer_count = 0;
    uint16_t color_buffer[OUTPUT_BUFFER_SIZE];
    memset(color_buffer, 0, sizeof(color_buffer));

    // Track current position and window bounds
    int16_t current_x = x0;
    int16_t current_y = y0;
    int16_t min_x = x0, max_x = x0;
    int16_t min_y = y0, max_y = y0;

    while (1) {
        if (current_x == x1 && current_y == y1) break;
        color_buffer[buffer_count++] = color;

        // When buffer is full, send all pixels
        if (buffer_count >= OUTPUT_BUFFER_SIZE) {
            // Update window bounds
            if (current_x < min_x) min_x = current_x;
            if (current_x > max_x) max_x = current_x;
            if (current_y < min_y) min_y = current_y;
            if (current_y > max_y) max_y = current_y;

            printf("window [%d, %d, %d, %d]\n", min_x, min_y, max_x, max_y);
            for (int i = 0; i < buffer_count; i++) {
                // print_hex_debug(color_buffer[i]);
                if ((i+1) % dx == 0) printf(".\n");
            }
            printf("\n\n");

            modTFT_setWindow(min_x, min_y, max_x, max_y, config);
            send_spi_data((uint8_t*)color_buffer, buffer_count * 2, config);
            
            // Reset for next segment
            buffer_count = 0;
            min_x = current_x;
            max_x = current_x;
            min_y = current_y;
            max_y = current_y;
        }

        e2 = err;
        if (e2 > -dx) {
            err -= dy;
            current_x += sx;
        }
        if (e2 < dy) {
            err += dx;
            current_y += sy;
        }
    }

    // Send remaining pixels
    if (buffer_count > 0) {
        printf("***window [%d, %d, %d, %d]\n", min_x, min_y, max_x, max_y);
        printf("New dx segment (Y change):\n");
        for (int i = 0; i < buffer_count; i++) {
            // print_hex_debug(color_buffer[i]);
            if ((i+1) % dx == 0) printf(".\n");
        }
        printf("\n\n");

        modTFT_setWindow(min_x, min_y, max_x, max_y, config);
        send_spi_data((uint8_t*)color_buffer, buffer_count * 2, config);
    }
}

//# Draw horizontal line
void st7735_draw_horLine(
    int y, int x0, int x1, 
    uint16_t color, int thickness, 
    M_Spi_Conf *config
) {
    if (x0 > x1) { int tmp = x0; x0 = x1; x1 = tmp; }

    static uint32_t chunk_buffer[(MAX_CHUNK_PIXELS + 1) / 2];

    // Pack 2 pixels per word
    uint32_t color32 = (color << 16) | color;
    for (int i = 0; i < sizeof(chunk_buffer)/sizeof(chunk_buffer[0]); i++) {
        chunk_buffer[i] = color32;  // Compiler may optimize this
    }

    // Draw chunks
    int chunk_bytes;
    for (int x = x0; x <= x1; x += MAX_CHUNK_PIXELS) {
        int width = MIN(MAX_CHUNK_PIXELS, x1 - x + 1);
        chunk_bytes = width * thickness * 2;

        modTFT_setWindow(x, y, x + width - 1, y + thickness - 1, config);
        send_spi_data((uint8_t*)chunk_buffer, chunk_bytes, config);
    }
}

//# Draw vertical line
void st7735_draw_verLine(
    int x, int y0, int y1,
    uint16_t color, int thickness,
    M_Spi_Conf *config
) {
    if (y0 > y1) { int tmp = y0; y0 = y1; y1 = tmp; }

    static uint32_t chunk_buffer[(MAX_CHUNK_PIXELS + 1) / 2];

    // Pack 2 pixels per word
    uint32_t color32 = (color << 16) | color;
    for (int i = 0; i < sizeof(chunk_buffer)/sizeof(chunk_buffer[0]); i++) {
        chunk_buffer[i] = color32;  // Compiler may optimize this
    }

    // Draw chunks
    int chunk_bytes;
    for (int y = y0; y <= y1; y += MAX_CHUNK_PIXELS) {
        int height = MIN(MAX_CHUNK_PIXELS, y1 - y + 1);
        chunk_bytes = height * thickness * 2;
        
        //# Set window & send data
        modTFT_setWindow(x, y, x + thickness - 1, y + height - 1, config);
        send_spi_data((uint8_t*)chunk_buffer, chunk_bytes, config);
    }
}

//# Draw Rectangle
void st7735_draw_rectangle(
    uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, 
    uint16_t color, uint8_t thickness, M_Spi_Conf *config
) {
    st7735_draw_horLine(y0, x0, x1, color, thickness, config);
    st7735_draw_horLine(y1, x0, x1, color, thickness, config);
    st7735_draw_verLine(x0, y0, y1, color, thickness, config);
    st7735_draw_verLine(x1, y0, y1, color, thickness, config);
}

#include <stdio.h>
#include <unistd.h>
#include <stdint.h>  
#include <string.h>
#include <stdlib.h>

#include "modTFT_line.h"


//# Draw Rectangle
void modTFT_drawRect(
    int x0, int y0, int x1, int y1, 
    uint16_t color, int thickness, M_Spi_Conf *config
) {
    modTFT_drawHorLine(y0, x0, x1, color, thickness, config);
    modTFT_drawHorLine(y1, x0, x1, color, thickness, config);
    modTFT_drawVerLine(x0, y0, y1, color, thickness, config);
    modTFT_drawVerLine(x1, y0, y1, color, thickness, config);
}


static void modTFT_drawCircle(
    int px, int py, int radius,
    uint16_t color, M_Spi_Conf *conf
) {
    int x = 0;
    int y = radius;
    int err = 1 - radius; // Initial error term

    while (x <= y) {
        // Draw symmetric points in all octants
        modTFT_drawPixel(px + x, py + y, color, conf); // Octant 1
        modTFT_drawPixel(px - x, py + y, color, conf); // Octant 2
        modTFT_drawPixel(px + x, py - y, color, conf); // Octant 3
        modTFT_drawPixel(px - x, py - y, color, conf); // Octant 4
        modTFT_drawPixel(px + y, py + x, color, conf); // Octant 5
        modTFT_drawPixel(px - y, py + x, color, conf); // Octant 6
        modTFT_drawPixel(px + y, py - x, color, conf); // Octant 7
        modTFT_drawPixel(px - y, py - x, color, conf); // Octant 8

        if (err < 0) {
            err += 2 * x + 3;
        } else {
            err += 2 * (x - y) + 5;
            y--;
        }
        x++;
    }
}

#include <math.h>

void modTFT_drawFilledCircle(
    int center_x, int center_y, int radius,
    uint16_t color, M_Spi_Conf *conf)
{
    // For each vertical line in the circle's bounding box
    for (int x = -radius; x <= radius; x++) {
        // Calculate vertical span using circle equation
        int h = (int)sqrt(radius * radius - x * x);
        int top_y = center_y - h;
        int bottom_y = center_y + h;
        
        // Draw vertical line from top to bottom
        modTFT_drawVerLine(
            center_x + x,  // x position
            top_y,         // start y (top)
            bottom_y,      // end y (bottom)
            color,         // color
            1,            // thickness
            conf          // SPI config
        );
    }
}

void modTFT_drawPoly(
    int *x_coords, int *y_coords, int num_verts,
    uint16_t color, int thickness, M_Spi_Conf *config
) {
    // Need at least 2 points to draw a polygon
    if (num_verts < 2) return;

    // Draw lines between consecutive vertices
    for (int i = 0; i < num_verts; i++) {
        int j = (i + 1) % num_verts; // Connect back to first vertex
        
        draw_line_bresenham(
            x_coords[i], y_coords[i],  // Start point
            x_coords[j], y_coords[j],  // End point
            color, thickness, config
        );
    }
}


#define MAX_POLY_SIDES 20

void modTFT_drawFilledPoly(
    int *x_coords, int *y_coords, int num_verts,
    uint16_t color, M_Spi_Conf *config
) {
    if (num_verts < 3) return;

    // 1. Find top/bottom bounds
    int ymin = y_coords[0], ymax = y_coords[0];

    for (int i = 1; i < num_verts; i++) {
        ymin = MIN(ymin, y_coords[i]);
        ymax = MAX(ymax, y_coords[i]);
    }

    // 2. For each scanline
    for (int scan_y = ymin; scan_y <= ymax; scan_y++) {
        int nodes[16]; // Stores x intersections
        int count = 0;
        
        // 3. Find edge intersections
        for (int i = 0, j = num_verts-1; i < num_verts; j = i++) {
            if ((y_coords[i] >= scan_y && y_coords[j] < scan_y) || 
                (y_coords[j] >= scan_y && y_coords[i] < scan_y)) {
                nodes[count++] = x_coords[i] + (int32_t)(scan_y - y_coords[i]) * 
                                    (x_coords[j] - x_coords[i]) / (y_coords[j] - y_coords[i]);
            }
        }

        // 4. Sort intersections (simple bubble sort)
        for (int i = 0; i < count-1; i++) {
            for (int j = i+1; j < count; j++) {
                if (nodes[i] > nodes[j]) {
                    int t = nodes[i];
                    nodes[i] = nodes[j];
                    nodes[j] = t;
                }
            }
        }

        // 5. Draw vertical lines between pairs
        for (int i = 0; i < count; i += 2) {
            if (i+1 >= count) break;
            modTFT_drawVerLine(
                nodes[i],    // x position
                scan_y,      // top y
                scan_y,      // bottom y (same line)
                color,
                nodes[i+1] - nodes[i] + 1, // thickness = width
                config
            );
        }
    }
}
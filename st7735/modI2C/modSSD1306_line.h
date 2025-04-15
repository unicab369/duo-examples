#include "modSSD1306.h"

//! prefill_horLine
void prefill_horLine(
	int y, int x0, int x1,
    int thickness, int mirror
) {
    // Validate coordinates
    if (y >= SSD1306_H) return;

	// Clamp to display bounds - branchless operation
    x0 = (x0 >= SSD1306_H) ? SSD1306_W_MASK : x0;
    x1 = (x1 >= SSD1306_H) ? SSD1306_W_MASK : x1;
    
	// Handle mirroring
	if (mirror) {
		x0 = SSD1306_W_MASK - x0;
		x1 = SSD1306_W_MASK - x1;
	}

	// Ensure x1 <= x2 (swap if needed) - branchless operation
    int x_min = x0 < x1 ? x0 : x1;
    int x_max = x0 < x1 ? x1 : x0;
    x0 = x_min;
    x1 = x_max;

    // calculte y_end - branchless operation
    int y_end  = y + thickness - 1;
    y_end = (y_end >= SSD1306_H) ? SSD1306_H : y_end;
	if (y_end < y) return;  // Skip if thickness causes overflow

    // Draw thick line
    for (int y_pos = y; y_pos <= y_end ; y_pos++) {
        M_Page_Mask mask = page_masks[y_pos];
        uint8_t *ref_page = frame_buffer[mask.page];
        const uint8_t bitmask = mask.bitmask;

        for (int x_pos = x0; x_pos <= x1; x_pos++) {
            ref_page[x_pos] |= bitmask;
        }
    }
}


//! prefill_verLine
void prefill_verLine(
	int x, int y0, int y1,
    int thickness, int mirror
) {
    // Validate coordinates
    if (x >= SSD1306_W) return;

	// Clamp to display bounds - branchless operation
    y0 = (y0 >= SSD1306_H) ? SSD1306_H_MASK : y0;
    y1 = (y1 >= SSD1306_H) ? SSD1306_H_MASK : y1;

	// Handle mirroring
	if (mirror) {
		y0 = SSD1306_H_MASK - y0;
		y1 = SSD1306_H_MASK - y1;
	}

	// Ensure y1 <= y2 (swap if needed) - branchless operation
    int y_min = y0 < y1 ? y0 : y1;
    int y_max = y0 < y1 ? y1 : y0;
    y0 = y_min;
    y1 = y_max;

    // calculte x_end - branchless operation
    int x_end = x + thickness - 1;
    x_end = (x_end >= SSD1306_W) ? SSD1306_W_MASK : x_end;
	if (x_end < x) return;  // Skip if thickness causes overflow

    // Draw vertical line with thickness
    for (int y_pos = y0; y_pos <= y1; y_pos++) {
        const M_Page_Mask mask = page_masks[y_pos];
        uint8_t *ref_page = frame_buffer[mask.page];
        const uint8_t bitmask = mask.bitmask;
        
        // Use pointer arithmetic and potentially unroll small loops
        for (int x_pos = x; x_pos <= x_end; x_pos++) {
            ref_page[x_pos] |= bitmask;
        }
    }
}

    // Avoid division by zero and optimize for thin lines
    // if (thickness <= 1) {
    //     // Fall back to simple Bresenham for thin lines
    //     prefill_bresenhamLine(x0, y0, x1, y1);
    //     return;
    // }

    
void prefill_line(int x0, int y0, int x1, int y1, int thickness) {
    // Early exit for zero-length lines
    if (x0 == x1 && y0 == y1) return;

    // Calculate direction and perpendicular vectors
    int dx = x1 - x0;
    int dy = y1 - y0;
    
    // Fast integer approximation of normalized perpendicular
    int px = -dy;
    int py = dx;
    int length_sq = px*px + py*py;
    


    // Approximate scaling factor (avoid sqrt)
    int scale = (thickness * 256) / (16 + (length_sq >> 8)); // Empirical adjustment
    px = (px * scale) >> 8;
    py = (py * scale) >> 8;

    // Calculate bounding box (optimized)
    int bounds[4] = {
        MIN(MIN(x0 + px, x0 - px), MIN(x1 - px, x1 + px)),
        MAX(MAX(x0 + px, x0 - px), MAX(x1 - px, x1 + px)),
        MIN(MIN(y0 + py, y0 - py), MIN(y1 - py, y1 + py)),
        MAX(MAX(y0 + py, y0 - py), MAX(y1 - py, y1 + py))
    };

    // Clamp to display bounds
    bounds[0] = CLAMP(bounds[0], 0, SSD1306_W-1);
    bounds[1] = CLAMP(bounds[1], bounds[0], SSD1306_W-1); // Ensure min <= max
    bounds[2] = CLAMP(bounds[2], 0, SSD1306_H-1);
    bounds[3] = CLAMP(bounds[3], bounds[2], SSD1306_H-1);

    // Precompute edge vectors
    int edge_x[4] = {x0 + px, x0 - px, x1 - px, x1 + px};
    int edge_y[4] = {y0 + py, y0 - py, y1 - py, y1 + py};

    // Scanline fill optimized for SSD1306
    for (int y_pos = bounds[2]; y_pos <= bounds[3]; y_pos++) {
        M_Page_Mask mask = page_masks[y_pos];
        uint8_t* row = &frame_buffer[mask.page][0];
        int x_start = SSD1306_W, x_end = 0;

        // Find x intersections with edges
        for (int i = 0; i < 4; i++) {
            int j = (i + 1) % 4;
            int y1 = edge_y[i], y2 = edge_y[j];
            
            if ((y_pos < y1) == (y_pos < y2)) continue;
            
            int x = edge_x[i] + (y_pos - y1) * (edge_x[j] - edge_x[i]) / (y2 - y1);
            x_start = MIN(x_start, x);
            x_end = MAX(x_end, x);
        }

        // Clamp and draw the span
        x_start = CLAMP(x_start, bounds[0], bounds[1]);
        x_end = CLAMP(x_end, x_start, bounds[1]);
        
        if (x_start <= x_end) {
            uint8_t* p = row + x_start;
            uint8_t* end = row + x_end;
            while (p <= end) *p++ |= mask.bitmask;
        }
    }
}


// void prefill_poly(M_Point *pts, uint8_t num_pts, uint8_t thickness) {
//     if (num_pts < 3) return;  // Need at least 3 points for a polygon
//     prefill_lines(pts, num_pts, thickness);
//     prefill_line(pts[num_pts-1], pts[0], thickness);
// }

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



// void prefill_poly(M_Point *pts, uint8_t num_pts, uint8_t thickness) {
//     if (num_pts < 3) return;  // Need at least 3 points for a polygon
//     prefill_lines(pts, num_pts, thickness);
//     prefill_line(pts[num_pts-1], pts[0], thickness);
// }




void prefill_line(int x0, int y0, int x1, int y1, int thickness) {
    // Calculate perpendicular vector (for thickness direction)
    int px = y0 - y1;
    int py = x1 - x0;
    
    // Normalize perpendicular vector
    int length = sqrt(px*px + py*py);
    if (length == 0) return;  // Zero-length line
    
    // Scale to half-thickness
    px = (px * thickness) / (2 * length);
    py = (py * thickness) / (2 * length);
    
    // Calculate the 4 corners of the thick line
    int x[4] = {x0 + px, x0 - px, x1 - px, x1 + px};
    int y[4] = {y0 + py, y0 - py, y1 - py, y1 + py};
    
    // Find bounding box
    int min_x = MIN(MIN(x[0], x[1]), MIN(x[2], x[3]));
    int max_x = MAX(MAX(x[0], x[1]), MAX(x[2], x[3]));
    int min_y = MIN(MIN(y[0], y[1]), MIN(y[2], y[3]));
    int max_y = MAX(MAX(y[0], y[1]), MAX(y[2], y[3]));
    
    // Clamp to display (branchless version)
    min_x = (min_x < 0) ? 0 : (min_x >= SSD1306_W) ? SSD1306_W_MASK : min_x;
    max_x = (max_x < 0) ? 0 : (max_x >= SSD1306_W) ? SSD1306_W_MASK : max_x;
    min_y = (min_y < 0) ? 0 : (min_y >= SSD1306_H) ? SSD1306_H_MASK : min_y;
    max_y = (max_y < 0) ? 0 : (max_y >= SSD1306_H) ? SSD1306_H_MASK : max_y;

    // Scanline fill the polygon
    for (int y_pos = min_y; y_pos <= max_y; y_pos++) {
        M_Page_Mask mask = page_masks[y_pos];
        uint8_t* row = &frame_buffer[mask.page][0];
        
        // Find intersections with edges (unchanged)
        int intersections[4];
        int count = 0;
        
        for (int i = 0; i < 4; i++) {
            int j = (i + 1) % 4;
            if ((y[i] > y_pos && y[j] > y_pos) || (y[i] < y_pos && y[j] < y_pos)) 
                continue;
                
            if (y[i] == y[j]) {  // Horizontal edge
                intersections[count++] = x[i];
                intersections[count++] = x[j];
            } else {
                int x_intersect = x[i] + (y_pos - y[i]) * (x[j] - x[i]) / (y[j] - y[i]);
                intersections[count++] = x_intersect;
            }
        }

        // --- OPTIMIZED SORTING & DRAWING ---
        if (count >= 2) {
            // Branchless sort for 2 or 4 intersections
            if (count == 2) {
                int start = intersections[0];
                int end   = intersections[1];
                // Branchless swap to ensure start <= end
                int tmp = (start > end) ? end : start;
                end     = (start > end) ? start : end;
                start   = tmp;
                
                // Clamp and draw (unchanged)
                start = CLAMP(start, 0, SSD1306_W-1);
                end   = CLAMP(end,   start, SSD1306_W-1);
                uint8_t* p = row + start;
                uint8_t* end_ptr = row + end;
                while (p <= end_ptr) *p++ |= mask.bitmask;
            }

            else if (count == 4) {
                // Load all 4 points
                int a = intersections[0], b = intersections[1];
                int c = intersections[2], d = intersections[3];
                
                // Sort pairs (branchless)
                int min_ab = MIN(a, b), max_ab = MAX(a, b);
                int min_cd = MIN(c, d), max_cd = MAX(c, d);
                
                // Merge pairs (branchless)
                int start1 = MIN(min_ab, min_cd);
                int end1   = MAX(min_ab, min_cd);
                int start2 = MIN(max_ab, max_cd);
                int end2   = MAX(max_ab, max_cd);
                
                // Clamp and draw spans
                start1 = CLAMP(start1, 0, SSD1306_W-1);
                end1   = CLAMP(end1,   start1, SSD1306_W-1);
                start2 = CLAMP(start2, 0, SSD1306_W-1);
                end2   = CLAMP(end2,   start2, SSD1306_W-1);
                
                // Fill spans
                uint8_t* p = row + start1;
                uint8_t* end_ptr = row + end1;
                while (p <= end_ptr) *p++ |= mask.bitmask;
                
                if (start2 > end1) {  // Non-overlapping spans only
                    p = row + start2;
                    end_ptr = row + end2;
                    while (p <= end_ptr) *p++ |= mask.bitmask;
                }
            }
        }
    }
}

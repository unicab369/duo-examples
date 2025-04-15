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
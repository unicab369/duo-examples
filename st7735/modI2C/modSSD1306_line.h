#include "modSSD1306.h"

//! prefill_horLine
void prefill_horLine(
	uint8_t y, uint8_t x0, uint8_t x1,
    uint8_t thickness, uint8_t mirror
) {
    // Validate coordinates
    if (y >= SSD1306_H) return;

	// Clamp to display bounds
    if (x0 >= SSD1306_W) x0 = SSD1306_W_LIMIT;
    if (x1 >= SSD1306_W) x1 = SSD1306_W_LIMIT;
    
	// Handle mirroring
	if (mirror) {
		x0 = SSD1306_W_LIMIT - x0;
		x1 = SSD1306_W_LIMIT - x1;
	}

	// Ensure x1 <= x2 (swap if needed)
	if (x0 > x1) SWAP_INT(x0, x1);

    // Handle thickness
    uint8_t y_end  = y + thickness - 1;
    if (y_end >= SSD1306_H) y_end = SSD1306_H_LIMIT;
	if (y_end < y) return;  // Skip if thickness is 0 or overflowed

    // Draw thick line
    for (uint8_t y_pos = y; y_pos <= y_end ; y_pos++) {
        M_Page_Mask mask = page_masks[y_pos];

        for (uint8_t x_pos = x0; x_pos <= x1; x_pos++) {
            frame_buffer[mask.page][x_pos] |= mask.bitmask;
        }
    }
}


//! prefill_verLine
void prefill_verLine(
	uint8_t x, uint8_t y0, uint8_t y1,
    uint8_t thickness, uint8_t mirror
) {
    // Validate coordinates
    if (x >= SSD1306_W) return;

	// Clamp to display bounds
    if (y0 >= SSD1306_H) y0 = SSD1306_H_LIMIT;
    if (y1 >= SSD1306_H) y1 = SSD1306_H_LIMIT;

	// Handle mirroring
	if (mirror) {
		y0 = SSD1306_H_LIMIT - y0;
		y1 = SSD1306_H_LIMIT - y1;
	}

	// Ensure y1 <= y2 (swap if needed)
    if (y0 > y1) SWAP_INT(y0, y1);

    // Handle thickness
    uint8_t x_end = x + thickness - 1;
    if (x_end >= SSD1306_W) x_end = SSD1306_W_LIMIT;
	if (x_end < x) return;  // Skip if thickness causes overflow

    // // Draw vertical line with thickness
    // for (uint8_t y_pos = y0; y_pos <= y1; y_pos++) {
    //     M_Page_Mask mask = page_masks[y_pos];
		
    //     for (uint8_t x_pos = x; x_pos <= x_end; x_pos++) {
    //         frame_buffer[mask.page][x_pos] |= mask.bitmask;
    //     }
    // }

	//# Optimized: save 500-700 us
	uint8_t x_len = x_end - x + 1;  // Precompute length

	for (uint8_t y_pos = y0; y_pos <= y1; y_pos++) {
		M_Page_Mask mask = page_masks[y_pos];
		uint8_t* row_start = &frame_buffer[mask.page][x];  	// Get row pointer

		for (uint8_t i = 0; i < x_len; i++) {
			row_start[i] |= mask.bitmask;  					// Sequential access
		}
	}
}
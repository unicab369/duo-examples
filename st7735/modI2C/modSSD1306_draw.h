#include "modSSD1306.h"

//! compute_fastPixel
void compute_fastPixel(int x, int y, int color) {
    M_Page_Mask mask = page_masks[y];

    if (color) {
        frame_buffer[mask.page][x] |= mask.bitmask;  // Set pixel (foreground)
    } else {
        frame_buffer[mask.page][x] &= ~mask.bitmask; // Clear pixel (background)
    }
}

//! compute_horLine
int compute_horLine(
	int y, int x0, int x1,
    int thickness, int mirror
) {
    // Validate coordinates
    if (y >= SSD1306_HEIGHT || thickness < 1) return -1;

	// Clamp to display bounds - branchless operation
    x0 = MIN(x0, SSD1306_WIDTH_MASK);
    x1 = MIN(x1, SSD1306_WIDTH_MASK);
    
	// Ensure x1 <= x2 - branchless operation
    x0 = MIN(x0, x1);
    x1 = MAX(x0, x1);

    // Handle mirroring
	if (mirror) {
		x0 = SSD1306_WIDTH_MASK - x0;
		x1 = SSD1306_WIDTH_MASK - x1;
	}

    // calculte y_end - branchless operation
    int y_end  = y + thickness - 1;
    y_end = MIN(y_end, SSD1306_HEIGHT_MASK);

    // Draw thick line
    for (int y_pos = y; y_pos <= y_end ; y_pos++) {
        M_Page_Mask mask = page_masks[y_pos];
        uint8_t *ref_page = frame_buffer[mask.page];
        const uint8_t bitmask = mask.bitmask;

        for (int x_pos = x0; x_pos <= x1; x_pos++) {
            ref_page[x_pos] |= bitmask;
        }
    }

    return 1;
}

//! compute_verLine
int compute_verLine(
	int x, int y0, int y1,
    int thickness, int mirror
) {
    // Validate coordinates
    if (x >= SSD1306_WIDTH || thickness < 1) return -1;

	// Clamp to display bounds - branchless operation
    y0 = MIN(y0, SSD1306_HEIGHT_MASK);
    y1 = MIN(y1, SSD1306_HEIGHT_MASK);

    // Ensure y1 <= y2 - branchless operation
    y0 = MIN(y0, y1);
    y1 = MAX(y0, y1);

	// Handle mirroring
	if (mirror) {
		y0 = SSD1306_HEIGHT_MASK - y0;
		y1 = SSD1306_HEIGHT_MASK - y1;
	}

    // calculte x_end - branchless operation
    int x_end = x + thickness - 1;
    x_end = MIN(x_end, SSD1306_WIDTH_MASK);

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

    return 1;
}

//! compute_diagLine
static int compute_diagLine(int x0, int y0, int x1, int y1, int color) {
    // Clamp coordinates
    x0 = CLAMP(x0, 0, SSD1306_WIDTH_MASK);
    y0 = CLAMP(y0, 0, SSD1306_HEIGHT_MASK);
    x1 = CLAMP(x1, 0, SSD1306_WIDTH_MASK);
    y1 = CLAMP(y1, 0, SSD1306_HEIGHT_MASK);

    // Use linear interpolation for perfect straightness
    int dx = x1 - x0;
    int dy = y1 - y0;
    int steps = MAX(abs(dx), abs(dy));
    
    if (steps == 0) { // Single point
        compute_fastPixel(x0, y0, color);
        return -1;
    }

    // Fixed-point step values (16.16 precision)
    int x_step = (dx << 16) / steps;
    int y_step = (dy << 16) / steps;
    
    int x = x0 << 16;
    int y = y0 << 16;
    
    for (int i = 0; i <= steps; i++) {
        int px = x >> 16;
        int py = y >> 16;
        
        if (px >= 0 && px < SSD1306_WIDTH_MASK && py >= 0 && py < SSD1306_HEIGHT_MASK) {
            compute_fastPixel(px, py, color);
        }
        
        x += x_step;
        y += y_step;
    }

    return 1;
}

//! Bresenham's line can create a bent-looking line
static int compute_bresenhamLine(int x0, int y0, int x1, int y1) {
    // Clamp coordinates to display bounds
    x0 = CLAMP(x0, 0, SSD1306_WIDTH_MASK);
    y0 = CLAMP(y0, 0, SSD1306_HEIGHT_MASK);
    x1 = CLAMP(x1, 0, SSD1306_WIDTH_MASK);
    y1 = CLAMP(y1, 0, SSD1306_HEIGHT_MASK);

    // Bresenham's algorithm setup
    int dx = abs(x1 - x0);
    int dy = -abs(y1 - y0);
    int sx = x0 < x1 ? 1 : -1;
    int sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;
    int e2;

    while (1) {
        // Set pixel if within bounds
        if (x0 >= 0 && x0 < SSD1306_WIDTH_MASK && y0 >= 0 && y0 < SSD1306_HEIGHT_MASK) {
            M_Page_Mask mask = page_masks[y0];
            frame_buffer[mask.page][x0] |= mask.bitmask;
        }

        // Check for end of line
        if (x0 == x1 && y0 == y1) break;

        e2 = 2 * err;
        if (e2 >= dy) err += dy; x0 += sx;      // X-axis step
        if (e2 <= dx) err += dx; y0 += sy;      // Y-axis step
    }

    return 1;
}

//! compute_line: Generalized line drawing function
//# TODO: Issue with line thickness
int compute_line(int x0, int y0, int x1, int y1, int thickness) {
    // Handle single point case
    if (x0 == x1 && y0 == y1 || thickness < 1) return -1;

    //! handle vertical line
    if (x0 == x1) {
        compute_verLine(x0, y0, y1, thickness, 0);
        return -1;
    }
    
    //! handle horizontal line
    if (y0 == y1) {
        compute_horLine(y0, x0, x1, thickness, 0);
        return -1;
    }

    //! handle single pixel thickness line
    if (thickness == 1) {
        compute_diagLine(x0, y0, x1, y1, 1);
        return -1;
    }
    
    // Calculate direction and perpendicular vectors
    int dx = x1 - x0;   //! always positive
    int dy = y1 - y0;   //! always positive
    int px = -dy;
    int py = dx;

    // Approximate thickness scaling (avoiding sqrt)
    int length_sq = px*px + py*py;
    int scale = (thickness * 256) / (16 + (length_sq >> 8));
    px = (px * scale) >> 8;
    py = (py * scale) >> 8;

    // Calculate edge points
    int edges_x[4] = {x0 + px, x0 - px, x1 - px, x1 + px};
    int edges_y[4] = {y0 + py, y0 - py, y1 - py, y1 + py};

    // Calculate and clamp bounding box
    int min_x = MIN(MIN(edges_x[0], edges_x[1]), MIN(edges_x[2], edges_x[3]));
    int max_x = MAX(MAX(edges_x[0], edges_x[1]), MAX(edges_x[2], edges_x[3]));
    int min_y = MIN(MIN(edges_y[0], edges_y[1]), MIN(edges_y[2], edges_y[3]));
    int max_y = MAX(MAX(edges_y[0], edges_y[1]), MAX(edges_y[2], edges_y[3]));

    min_x = CLAMP(min_x, 0      , SSD1306_WIDTH_MASK);
    max_x = CLAMP(max_x, min_x  , SSD1306_WIDTH_MASK);
    min_y = CLAMP(min_y, 0      , SSD1306_HEIGHT_MASK);
    max_y = CLAMP(max_y, min_y  , SSD1306_HEIGHT_MASK);

    // Scanline fill
    for (int y = min_y; y <= max_y; y++) {
        M_Page_Mask mask = page_masks[y];
        uint8_t* row = &frame_buffer[mask.page][0];
        int x_min = SSD1306_WIDTH_MASK, x_max = 0;

        // Find intersections with all edges
        for (int i = 0; i < 4; i++) {
            int j = (i + 1) % 4;
            int y1 = edges_y[i], y2 = edges_y[j];
            
            // Skip if edge doesn't intersect scanline
            if ((y < y1) == (y < y2)) continue;
            
            // Calculate intersection point
            int x = edges_x[i] + (y - y1) * (edges_x[j] - edges_x[i]) / (y2 - y1);
            x_min = MIN(x_min, x);
            x_max = MAX(x_max, x);
        }

        // Clamp and draw the horizontal span
        x_min = CLAMP(x_min, min_x, max_x);
        x_max = CLAMP(x_max, x_min, max_x);
        
        if (x_min <= x_max) {
            uint8_t* p = row + x_min;
            uint8_t* end = row + x_max;
            while (p <= end) *p++ |= mask.bitmask;
        }
    }

    return 1;
}


//! compute_fastHorLine
static int compute_fastHorLine(int y, int x0, int x1, int color) {
    if (y >= SSD1306_HEIGHT) return -1;
    
	// Clamp x-coordinates - branchless operation
    x0 = MIN(x0, SSD1306_WIDTH_MASK);
    x1 = MIN(x1, SSD1306_WIDTH_MASK);

    M_Page_Mask mask = page_masks[y];

    for (int x = x0; x <= x1; x++) {
        if (color) {
            frame_buffer[mask.page][x] |= mask.bitmask;  // Set pixel (foreground)
        } else {
            frame_buffer[mask.page][x] &= ~mask.bitmask; // Clear pixel (background)
        }
    }

    return 1;
}

//! compute_rect
int compute_rect(int x0, int y0, int x1, int y1, int thickness) {
    // Draw top and bottom lines
    if (compute_horLine(y0, x0, x1, thickness, 0) < 0) return -1;
    if (compute_horLine(y1, x0, x1, thickness, 0) < 0) return -2;

    // Draw left and right lines
    if (compute_verLine(x0, y0, y1, thickness, 0) < 0) return -3;
    if (compute_verLine(x1, y0, y1, thickness, 0) < 0) return -4;

    return 1;
}

//! compute_filledRect
int compute_filledRect(int x0, int y0, int x1, int y1) {
    // Validate and order coordinates
    int x_min = MIN(x0, x1);
    int x_max = MAX(x0, x1);
    int y_min = MIN(y0, y1);
    int y_max = MAX(y0, y1);

    // Clamp coordinates to display boundaries
    x_min = CLAMP(x_min, 0, SSD1306_WIDTH_MASK);
    x_max = CLAMP(x_max, 0, SSD1306_WIDTH_MASK);
    y_min = CLAMP(y_min, 0, SSD1306_HEIGHT_MASK);
    y_max = CLAMP(y_max, 0, SSD1306_HEIGHT_MASK);
    
    // Draw filled rectangle using horizontal lines
    for (int y = y_min; y <= y_max; y++) {
        if (compute_fastHorLine(y, x_min, x_max, 1) < 0) return -1;
    }
    
    return 1;  // Success
}

//! compute_multiLines
void compute_multiLines(int *xs, int *ys, int num_pts, int thickness) {
    if (num_pts < 2) return;  // Need at least 2 points for a line

    for (int i = 0; i < num_pts - 1; i++) {
        compute_line(xs[i], ys[i], xs[i + 1], ys[i + 1], thickness);
    }
}

//! compute_poly
void compute_poly(int *xs, int *ys, int num_pts, int thickness) {
    if (num_pts < 3) return;  // Need at least 3 points for a polygon

    // Draw lines between each pair of points
    for (int i = 0; i < num_pts - 1; i++) {
        compute_line(xs[i], ys[i], xs[i + 1], ys[i + 1], thickness);
    }
    
    // Close the polygon by drawing a line from the last point to the first
    compute_line(xs[num_pts - 1], ys[num_pts - 1], xs[0], ys[0], thickness);
}

//! compute_circleLine
static int compute_circleLine(
	int x0, int y0, int radius, int color
) {
	// Validate center coordinates
	if (x0 >= SSD1306_WIDTH_MASK + radius ||
        y0 >= SSD1306_HEIGHT_MASK + radius || radius < 1) return -1;

    int x = -radius;
    int y = 0;
    int err = 2 - 2 * radius;
	int e2;

    do {
        // Calculate endpoints with clamping
        int x_start 	= x0 + x;
        int x_end   	= x0 - x;
        int y_top   	= y0 - y;
        int y_bottom 	= y0 + y;

        int xy_start 	= x0 + y;
        int xy_end   	= x0 - y;
        int yx_start 	= y0 + x;
        int yx_end   	= y0 - x;

        // Draw all 8 symmetric points (using precomputed page_masks)
        compute_fastPixel(x_end		, y_bottom  , color); 	    // Octant 1
        compute_fastPixel(x_start	, y_bottom  , color); 	    // Octant 2
        compute_fastPixel(x_start	, y_top     , color); 		// Octant 3
        compute_fastPixel(x_end		, y_top     , color); 		// Octant 4
        compute_fastPixel(xy_end	, yx_start  , color); 	    // Octant 5
        compute_fastPixel(xy_start	, yx_start  , color); 	    // Octant 6
        compute_fastPixel(xy_start	, yx_end    , color); 		// Octant 7
        compute_fastPixel(xy_end	, yx_end    , color); 		// Octant 8

        // Update Bresenham error
		e2 = err;
        if (e2 <= y) {
            err += ++y * 2 + 1;
            if (-x == y && e2 <= x) e2 = 0;
        }
        if (e2 > x) err += ++x * 2 + 1;
    } while (x <= 0);

    return 1;
}

//! compute_filledCircle
void compute_filledCircle(
    int x0, int y0, int radius, int color
) {
	// Validate center coordinates
	if (x0 >= SSD1306_WIDTH_MASK + radius ||
        y0 >= SSD1306_HEIGHT_MASK + radius || radius < 1) return;

    int x = -radius;
    int y = 0;
    int err = 2 - 2 * radius;
	int e2;

    do {
        // Calculate endpoints with clamping
        int x_start 	= x0 + x;
        int x_end   	= x0 - x;
        int y_top   	= y0 - y;
        int y_bottom 	= y0 + y;

        // Draw filled horizontal lines (top and bottom halves)
        compute_fastHorLine(y_top, x_start, x_end, color);     // Top half
        compute_fastHorLine(y_bottom, x_start, x_end, color);  // Bottom half

        // Update Bresenham error
		e2 = err;
        if (e2 <= y) {
            err += ++y * 2 + 1;
            if (-x == y && e2 <= x) e2 = 0;
        }
        if (e2 > x) err += ++x * 2 + 1;
    } while (x <= 0);
}

//! compute_circle
int compute_circle(int x0, int y0, int radius, int thickness) {
    // Validate parameters
    if (radius == 0 || thickness < 1 || thickness > radius) return -1;

    if (thickness == 1) {
        compute_circleLine(x0, y0, radius, 1);
    }

    compute_filledCircle(x0, y0, radius, 1);                // First draw the outer filled circle
    compute_filledCircle(x0, y0, radius - thickness, 0);    // Then subtract the inner circle
    return 1;
}

//! compute_str_atLine
void compute_str_atLine(uint8_t page, uint8_t column, const char *str) {
    uint8_t current_column = column;
    
    while (*str && current_column < SSD1306_WIDTH - 6) { // Leave space for 5 pixel width + 1 pixel spacing
        uint8_t char_index = *str - 32; // Adjust for ASCII offset

        // Copy font data to frame buffer
        memcpy(&frame_buffer[page][current_column], &FONT_7x5[char_index], 5);
        
        current_column += 6; // Move to the next character position (5 pixels + 1 spacing)
        str++;
    }
}

typedef struct {
    const uint8_t *FONT;
    int font_width;
    int font_height;
    int scale, spacing;
    uint8_t color;
} M_Text;

//! compute_char
static void compute_char(int x, int y, char chr, M_Text *model) {
    // Early returns for invalid cases
    if (model->scale == 0 || chr < 32 || chr > 126 || 
        x >= SSD1306_WIDTH || y >= SSD1306_HEIGHT) return;

    const uint8_t *char_data = model->FONT + ((chr - 32) * model->font_width);
    uint8_t height_mask = model->font_height-1;

    for (uint8_t col = 0; col < model->font_width; col++) {
        uint8_t font_col = char_data[col];
        uint8_t px_base = x + col * model->scale;

        // Process all 7 rows
        for (uint8_t row = 0; row < model->font_height; row++) {
            if (font_col & (1 << (height_mask - row))) {
                uint8_t py_base = y + (height_mask - row) * model->scale;
                
                // Optimized inner scaling loop
                uint8_t px_end = px_base + model->scale;
                uint8_t py_end = py_base + model->scale;
                
                for (uint8_t px = px_base; px < px_end && px < SSD1306_WIDTH; px++) {
                    for (uint8_t py = py_base; py < py_end && py < SSD1306_HEIGHT; py++) {
                        compute_fastPixel(px, py, model->color);
                    }
                }
            }
        }
    }
}

//! compute_string
void compute_string(int x, int y, char *str, M_Text *model) {
    int char_width = model->scale * (model->font_width + model->spacing);
    int x_max = SSD1306_WIDTH - char_width;

    while (*str && x < x_max) {
        compute_char(x, y, *str, model);
        x += char_width;
        str++;
    }
}


void test_clearScreen1() {
    modSSD1306_clearScreen(0xFF);
}

void test_horLine() {
    compute_horLine(10, 0, SSD1306_WIDTH, 3, 0);
}

void test_verLine() {
    compute_verLine(10, 0, SSD1306_HEIGHT, 3, 0);
}

void test_diagLine() {
    compute_line(0, 0, SSD1306_HEIGHT, SSD1306_HEIGHT, 1);
}

void test_rect() {
    compute_rect(20, 20, 40, 40, 2);
}

void test_fillRect() {
    compute_filledRect(50, 50, 65, 65);
}

void shift_value(int shift, int *input, int len) {
    for(int i=0; i<len; i++) {
        input[i] += shift;
    }
}

void test_hexagon() {
    int x_points[] = { 12, 16, 24, 18, 22, 12, 2, 6, 0, 8 };
    shift_value(80, x_points, sizeof(x_points)/sizeof(int));
    
    int y_points[] = { 0, 8, 8, 14, 22, 16, 22, 14, 8, 8 };
    shift_value(30, y_points, sizeof(x_points)/sizeof(int));
    
    compute_poly(x_points, y_points, 10, 1);
}

void test_circle() {
    compute_circle(60, 20, 10, 1);
}

void test_filledCircle() {
    compute_filledCircle(60, 40, 10, 1);
}

void test_string() {
    compute_str_atLine(0, 0, "Hello Bee!");
}

void test_string2() {
    M_Text textModel = {
        .FONT = (const uint8_t *)FONT_7x5,
        .font_width = 5,
        .font_height = 7,
        .scale = 2,
        .spacing = 1,
        .color = 1
    };

    compute_string(0, 20, "Hello Bee!",  &textModel);
}


typedef struct {
    int channel;
    int host;
    int address;
    int last_change_time;
    int print_log;
} M_SSD1306;

void test_ssd1306_draw(M_SSD1306 *model) {
    uint32_t now = millis();
    if (now - model->last_change_time < 1000) return;
    model->last_change_time = now;

    //! set host before writting data
    // printf("draw on host:\t\t %d\n", ssd1306->host);
    ssd1306_setHost(model->host);
    modSSD1306_clearScreen(0x00);
    
    // print_elapse_nanoSec("test_clearScreen1", test_clearScreen1, print_log);

    print_elapse_nanoSec("test_horLine", test_horLine, model->print_log);

    print_elapse_nanoSec("test_verLine", test_verLine, model->print_log);

    print_elapse_nanoSec("test_diagLine", test_diagLine, model->print_log);

    print_elapse_nanoSec("test_rect", test_rect, model->print_log);

    print_elapse_nanoSec("test_filRect", test_fillRect, model->print_log);

    print_elapse_nanoSec("test_hexagon", test_hexagon, model->print_log);

    print_elapse_nanoSec("test_circle", test_circle, model->print_log);

    print_elapse_nanoSec("test_filCircle", test_filledCircle, model->print_log);

    print_elapse_nanoSec("test_string", test_string, model->print_log);

    print_elapse_nanoSec("test_string2", test_string2, model->print_log);

    print_elapse_microSec("renderFrame", ssd1306_renderFrame, model->print_log);
}
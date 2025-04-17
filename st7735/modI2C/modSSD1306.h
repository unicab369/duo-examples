#include <wiringx.h>

#define SSD1306_WIDTH 128
#define SSD1306_HEIGHT 64

#define SSD1306_PAGES       (SSD1306_HEIGHT / 8) // 8 pages for 64 rows
// #define SSD1306_PAGES       1 // 8 pages for 64 rows

#define SSD1306_WIDTH_MASK	SSD1306_WIDTH - 1
#define SSD1306_HEIGHT_MASK	SSD1306_HEIGHT - 1

#define OLED_CMD     0
#define OLED_DATA    1

#define PAGE_SIZE    8
#define YLevel       0xB0
#define	Brightness	 0xFF 
#define WIDTH 	     128
#define HEIGHT 	     32

typedef struct {
    int host;
    int addr;
} M_I2C_Device;

int selected_host = -1;

void ssd1306_setHost(int host) {
    selected_host = host;
}

void ssd1306_writeCmds(
    uint8_t *commands, int length
) {
    for (int i = 0; i < length; i++) {
        wiringXI2CWriteReg8(selected_host, 0x00, commands[i]);
    }
}

static void ssd1306_writeData(
    uint8_t reg, uint8_t *buff, int len
) {
    int outputLen = len + 1;
    uint8_t data[outputLen];
	data[0] = reg;

	memcpy(&data[1], buff, len);
	int rc = write(selected_host, data, outputLen);
	if (rc != outputLen) {};
}

void ssd1306_setWindow(
    uint8_t start_page, uint8_t end_page,
    uint8_t start_column, uint8_t end_column
) {
    uint8_t columnCmds[] = { 0x21, start_column, end_column };
    ssd1306_writeCmds(columnCmds, sizeof(columnCmds));

    uint8_t pageCmds[] = { 0x22, start_page, end_page };
    ssd1306_writeCmds(pageCmds, sizeof(pageCmds));
}

typedef struct {
	uint8_t page;
	uint8_t bitmask;
} M_Page_Mask;

M_Page_Mask page_masks[SSD1306_HEIGHT];

uint8_t frame_buffer[SSD1306_PAGES][SSD1306_WIDTH] = { 0 };

void precompute_page_masks() {
	for (uint8_t y = 0; y < SSD1306_HEIGHT; y++) {
		page_masks[y].page       = y >> 3;             // (y / 8)
		page_masks[y].bitmask    = 1 << (y & 0x07);    // (y % 8)
	}
}

//! render area
void ssd1306_renderArea(
	uint8_t start_page, uint8_t end_page,
	uint8_t col_start, uint8_t col_end
) {
	ssd1306_setWindow(start_page, end_page, col_start, col_end-1);

    // writeMulti(0x40, &frame_buffer[start_page][col_start], sizeof(frame_buffer));
    for (uint8_t page = start_page; page <= end_page; page++) {
        ssd1306_writeData(0x40, &frame_buffer[page][col_start], col_end - col_start);
    }
}

//! render the entire screen
void ssd1306_renderFrame() {
	ssd1306_renderArea(0, 7, 0, SSD1306_WIDTH);
}

void modSSD1306_clearScreen(uint8_t color) {
    memset(frame_buffer, color, sizeof(frame_buffer)); // Clear the frame buffer
    ssd1306_renderFrame();
}

int ssd1306_init(int host_id) {
    //! set host before writing commands
    ssd1306_setHost(host_id);

    uint8_t ssd1306_setup_cmd[] = {
        0xAE, // Display off
        0xD5, // Set display clock divide
        0x80,
        0xA8, // Set multiplex ratio
        0x3F,
        0xD3, // Set display offset
        0x00,
        0x40, // Set start line
        0x8D, // Charge pump
        0x14,
        0x20, // Memory mode
        0x00,
        0xA1, // Segment remap
        0xC8, // COM output scan direction
        0xDA, // COM pins hardware
        0x12,
        0x81, // Contrast control
        0xCF,
        0xD9, // Pre-charge period
        0xF1,
        0xDB, // VCOMH deselect level
        0x30,
        0xA4, // Display resume
        0xA6, // Normal display
        0xAF, // Display on
    };
    
    ssd1306_writeCmds(ssd1306_setup_cmd, sizeof(ssd1306_setup_cmd));

    precompute_page_masks();

    modSSD1306_clearScreen(0); // Clear the screen

    return 1;
} 
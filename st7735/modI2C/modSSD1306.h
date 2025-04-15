#define SSD1306_W 128
#define SSD1306_H 64

#define SSD1306_PAGES       (SSD1306_H / 8) // 8 pages for 64 rows
// #define SSD1306_PAGES       1 // 8 pages for 64 rows

#define SSD1306_WIDTH_MASK	SSD1306_W - 1
#define SSD1306_HEIGHT_MASK	SSD1306_H - 1


int fd_i2c;

#define OLED_CMD     0
#define OLED_DATA    1

#define PAGE_SIZE    8
#define YLevel       0xB0
#define	Brightness	 0xFF 
#define WIDTH 	     128
#define HEIGHT 	     32	

void modI2C_writeCmds(uint8_t *commands, int length) {
    for (int i = 0; i < length; i++) {
        wiringXI2CWriteReg8(fd_i2c, 0x00, commands[i]);
    }
}

void ssd1306_write_byte(unsigned dat, unsigned cmd) {
	if(cmd) {
		wiringXI2CWriteReg8(fd_i2c, 0x40, dat);
	} else {
		wiringXI2CWriteReg8(fd_i2c, 0x00, dat);
	}
}

void ssd1306_set_position(unsigned char x, unsigned char y)  {
 	ssd1306_write_byte(YLevel+y, OLED_CMD);
	ssd1306_write_byte(((x&0xf0)>>4)|0x10, OLED_CMD);
	ssd1306_write_byte((x&0x0f), OLED_CMD); 
}

void ssd1306_push_char(uint8_t x,uint8_t y,uint8_t chr,uint8_t sizey) {
	uint8_t c=0, sizex=sizey/2;
	uint16_t i=0, size1;

	if (sizey==8) size1=6;
	else size1 = (sizey/8+ ((sizey%8) ? 1:0)) * (sizey/2);
	
    c=chr-' ';
	ssd1306_set_position(x,y);
	
    for(i=0; i<size1; i++) {
		if (i%sizex == 0 && sizey!=8) ssd1306_set_position(x, y++);
		if (sizey == 8) ssd1306_write_byte(asc2_0806[c][i], OLED_DATA);
		else if (sizey==16) ssd1306_write_byte(asc2_1608[c][i], OLED_DATA);
		else return;
	}
}

void ssd1306_push_string(uint8_t x,uint8_t y,uint8_t *chr,uint8_t sizey) {
	uint8_t j=0;

	while (chr[j]!='\0') {	
		ssd1306_push_char(x, y, chr[j++], sizey);
		if (sizey==8) x += 6;
		else x += sizey/2;
	}
}

void ssd1306_setWindow(
    uint8_t start_page, uint8_t end_page,
    uint8_t start_column, uint8_t end_column
) {
    uint8_t columnCmds[] = { 0x21, start_column, end_column };
    modI2C_writeCmds(columnCmds, sizeof(columnCmds));

    uint8_t pageCmds[] = { 0x22, start_page, end_page };
    modI2C_writeCmds(pageCmds, sizeof(pageCmds));
}

static void writeMulti(uint8_t reg, uint8_t *buff, int len) {
    int outputLen = len + 1;
    uint8_t data[outputLen];
	data[0] = reg;

	memcpy(&data[1], buff, len);
	int rc = write(fd_i2c, data, outputLen);
	if (rc != outputLen) {};
}


typedef struct {
	uint8_t page;
	uint8_t bitmask;
} M_Page_Mask;

M_Page_Mask page_masks[SSD1306_H];

uint8_t frame_buffer[SSD1306_PAGES][SSD1306_W] = { 0 };

void precompute_page_masks() {
	for (uint8_t y = 0; y < SSD1306_H; y++) {
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
        writeMulti(0x40, &frame_buffer[page][col_start], col_end - col_start);
    }
}

//! render the entire screen
void ssd1306_renderFrame() {
	ssd1306_renderArea(0, 7, 0, SSD1306_W);
}

void modSSD1306_clearScreen(uint8_t color) {
    memset(frame_buffer, color, sizeof(frame_buffer)); // Clear the frame buffer
    ssd1306_renderFrame();
}

void ssd1306_init(void) {
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
    
    modI2C_writeCmds(ssd1306_setup_cmd, sizeof(ssd1306_setup_cmd));

    precompute_page_masks();

    modSSD1306_clearScreen(0); // Clear the screen
} 
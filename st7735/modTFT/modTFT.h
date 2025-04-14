#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <stdlib.h>

#include "../utility.h"

#define CHUNK_SIZE 2000

#define ST7735_WIDTH 128
#define ST7735_HEIGHT 160

#define PURPLE      0x8000
#define GREEN       0x07E0
#define BLUE        0x001F
#define RED         0xF800
#define WHITE       0xFFFF
#define BLACK       0x0000
#define YELLOW      0xFFE0
#define CYAN        0x07FF
#define MAGENTA     0xF81F
#define ORANGE      0xFBE0
#define BROWN       0xA145
#define GRAY        0x7BEF
#define LIGHT_GRAY  0xC618
#define DARK_GRAY   0x7BEF

static uint16_t BACKGROUND_COLOR = 0x0000;

typedef struct {
    uint8_t HOST;
    int8_t MOSI;
    int8_t MISO;
    int8_t CLK;
    int8_t CS;      //! the main cs pin

    //! dc and rst are not part of SPI interface
    //! They are used for some display modules
    int8_t DC;          // set to -1 when not use
    int8_t RST;         // set to -1 when not use
} M_Spi_Conf;

static void spi_send_cmd(uint8_t cmd, M_Spi_Conf *conf) {
    digitalWrite(conf->DC, 0);      // Command mode
    wiringXSPIDataRW(conf->HOST, &cmd, 1);
}

int spi_send_data(uint8_t *data, int len, M_Spi_Conf *conf) {
    digitalWrite(conf->DC, 1);      // Data mode
    wiringXSPIDataRW(conf->HOST, data, len);
    return 1;
}

int spi_send_cmdData(uint8_t cmd, uint8_t *data, int len, M_Spi_Conf *conf) {
    spi_send_cmd(cmd, conf);
    spi_send_data(data, len, conf);
    return 1;
}

void modTFT_setWindow(
    uint8_t x0, uint8_t y0,
    uint8_t x1, uint8_t y1, M_Spi_Conf *conf
) {
    uint8_t data[] = {0x00, x0, 0x00, x1};

    //! Column Address Set: CASET
    spi_send_cmdData(0x2A, data, 4, conf);

    //! Row Address Set: RASET
    data[1] = y0;
    data[3] = y1;
    spi_send_cmdData(0x2B, data, 4, conf);

    //! Memory Write: RAMWR
    spi_send_cmd(0x2C, conf);        // Memory Write
}

void modTFT_drawPixel(
    int x, int y, uint16_t color, M_Spi_Conf *conf
) {
    modTFT_setWindow(x, y, x, y, conf);

    uint8_t data[2] = {
        (uint8_t)(color >> 8),  // High byte
        (uint8_t)(color & 0xFF) // Low byte
    };

    // print_hex(data, sizeof(data));

    spi_send_data(data, sizeof(data), conf);
}

void modTFT_fillRect(
    uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1,
    uint16_t color, M_Spi_Conf *conf
) {
    //# Set CS
    digitalWrite(conf->CS, 0);

    //! Set the address window to cover the entire screen
    modTFT_setWindow(x0, y0, x1, y1, conf);

    // Precompute the high and low bytes of the color
    uint8_t color_high = color >> 8;
    uint8_t color_low = color & 0xFF;
    uint8_t chunk[CHUNK_SIZE];              // Buffer to hold the chunk

    // Calculate the total number of pixels
    int total_pixels = (abs(x1 - x0) + 1) * (abs(y1 - y0) + 1);

    // Fill the screen in chunks
    int pixels_sent = 0;
    while (pixels_sent < total_pixels) {
        // Determine the number of pixels to send in this chunk
        int pixels_in_chunk = MIN(CHUNK_SIZE / 2, total_pixels - pixels_sent);

        // Fill the chunk with the color data
        for (int i = 0; i < pixels_in_chunk; i++) {
            chunk[2 * i] = color_high;   // High byte
            chunk[2 * i + 1] = color_low; // Low byte
        }

        //! Send the chunk to the display
        spi_send_data(chunk, pixels_in_chunk * 2, conf);

        //! Update the number of pixels sent
        pixels_sent += pixels_in_chunk;
    }
    
    //# Release CS
    digitalWrite(conf->CS, 1);

}

void modTFT_fillAll(uint16_t color, M_Spi_Conf *conf) {
    modTFT_fillRect(0, 0, ST7735_WIDTH - 1, ST7735_HEIGHT - 1, color, conf);
}

void modTFT_init(M_Spi_Conf *conf) {
    if (conf->CS > 0) pinMode(conf->CS, PINMODE_OUTPUT);
    if (conf->DC > 0) pinMode(conf->DC, PINMODE_OUTPUT);

    if (conf->RST > 0) {
        pinMode(conf->RST, PINMODE_OUTPUT);
        digitalWrite(conf->RST, HIGH);

        digitalWrite(conf->RST, LOW);
        // delayMicroseconds(10);
        digitalWrite(conf->RST, HIGH);
    }

    //# Set CS
    digitalWrite(conf->CS, 0);

    //! initializeation commands
    spi_send_cmd(0x01, conf);           // Software reset
    spi_send_cmd(0x11, conf);           // Sleep out
    
    // Set color mode: 16-bit color (RGB565)
    spi_send_cmdData(0x3A, (uint8_t[]){0x05}, 1, conf);

    spi_send_cmd(0x20, conf); // Inversion off
    // send_spi_cmd(0x21, conf); // Inversion on
    
    spi_send_cmd(0x29, conf); // Display on

    //# Release CS
    digitalWrite(conf->CS, 1);
}


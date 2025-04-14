#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <stdlib.h>


#include "utility.h"

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

static void send_spi_cmd(uint8_t cmd, M_Spi_Conf *conf) {
    digitalWrite(conf->DC, 0);      // Command mode
    wiringXSPIDataRW(conf->HOST, &cmd, 1);
}

int send_spi_data(uint8_t *data, int len, M_Spi_Conf *conf) {
    digitalWrite(conf->DC, 1);      // Data mode
    wiringXSPIDataRW(conf->HOST, data, len);
    return 1;
}

int spi_send_cmdData(uint8_t cmd, uint8_t *data, int len, M_Spi_Conf *conf) {
    send_spi_cmd(cmd, conf);
    send_spi_data(data, len, conf);
    return 1;
}

void modTFT_setWindow(
    uint8_t x0, uint8_t y0,
    uint8_t x1, uint8_t y1, M_Spi_Conf *conf
) {
    uint8_t data[] = {0x00, x0, 0x00, x1};

    //# Set CS
    digitalWrite(conf->CS, 0);

    //! Column Address Set: CASET
    spi_send_cmdData(0x2A, data, 4, conf);

    //! Row Address Set: RASET
    data[1] = y0;
    data[3] = y1;
    spi_send_cmdData(0x2B, data, 4, conf);

    //! Memory Write: RAMWR
    send_spi_cmd(0x2C, conf);        // Memory Write

    //# Release CS
    digitalWrite(conf->CS, 1);
}

void modTFT_drawPixel(int x, int y, uint16_t color, M_Spi_Conf *conf) {
    modTFT_setWindow(x, y, x, y, conf);

    uint8_t data[2] = {
        (uint8_t)(color >> 8),  // High byte
        (uint8_t)(color & 0xFF) // Low byte
    };

    send_spi_data(data, sizeof(data), conf);
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
        send_spi_data(chunk, pixels_in_chunk * 2, conf);

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
    send_spi_cmd(0x01, conf);           // Software reset
    send_spi_cmd(0x11, conf);           // Sleep out
    
    // Set color mode: 16-bit color (RGB565)
    spi_send_cmdData(0x3A, (uint8_t[]){0x05}, 1, conf);

    send_spi_cmd(0x20, conf); // Inversion off
    // send_spi_cmd(0x21, conf); // Inversion on
    
    send_spi_cmd(0x29, conf); // Display on

    //# Release CS
    digitalWrite(conf->CS, 1);
}


#define MAX_CHAR_COUNT 10

typedef struct {
    uint16_t line_idx;          // Current line index
    uint8_t current_y;          // Current y position

    uint8_t char_buff[MAX_CHAR_COUNT]; // Buffer to store accumulated character data
    uint8_t char_count;    // Number of characters accumulated
    uint8_t x0;     // Starting x position of the current buffer
} M_Render_State;


M_Render_State render_state = {
    .line_idx = 0,
    .char_count = 0
};

typedef struct {
    uint8_t x;
    uint8_t y;
    uint16_t color;

    //! page_wrap == 0 will neglect all character (if get outside of the window)
    //! and stop printing them out
    uint8_t page_wrap;

    //! word_wrap == 0 will wrap the cutoff characters (if get outside of the window)
    //! to the next line 
    //# word_wrap == 1 will wrap the whole word (if get outside of the window)
    //# to the next line
    uint8_t word_wrap;

    const uint8_t *font;    // Pointer to the font data
    uint8_t font_width;     // Width of each character in the font
    uint8_t font_height;    // Height of each character in the font
    uint8_t char_spacing;   // Spacing between characters (default: 1 pixel)
    const char* text;
} M_TFT_Text;

static void map_char_buffer(M_TFT_Text *model, uint16_t *my_buff, char c, uint8_t start_col, uint8_t total_cols) {
    //! Get the font data for the current character
    const uint8_t *char_data = &model->font[(c - 32) * model->font_width * ((model->font_height + 7) / 8)];

    //! Render the character into the frame buffer
    for (int j = 0; j < model->font_height; j++) { // Rows
        for (int i = 0; i < model->font_width; i++) { // Columns
            // Calculate the index in the flat buffer
            int index = (start_col + i) + j * total_cols;

            if (char_data[(i + (j / 8) * model->font_width)] & (1 << (j % 8))) {
                //! Pixel is part of the character (foreground color)
                my_buff[index] |= model->color; // RGB565 color
            } else {
                //! Pixel is not part of the character (background color)
                my_buff[index] |= BACKGROUND_COLOR; // RGB565 background color
            }
        }
    }
}


static void send_text_buffer(M_TFT_Text *model, M_Spi_Conf *config) {
    if (render_state.char_count <= 0) return;

    uint16_t spacing = model->char_spacing;
    uint8_t char_width = model->font_width + spacing;
    uint8_t font_height = model->font_height;

    uint16_t arr_width = char_width * render_state.char_count;
    uint16_t buff_len = arr_width * font_height;

    uint16_t *frame_buff = (uint16_t*)malloc(buff_len * sizeof(uint16_t));
    if (!frame_buff) {
        fprintf(stderr, "Memory allocation failed!\n");
        return; // Error handling
    }

    //! Initialize buffer to 0 (background color)
    memset(frame_buff, 0, sizeof(frame_buff));

    //! Render each character into the buffer
    for (uint8_t i = 0; i < render_state.char_count; i++) {
        // Calculate the starting column for the current character
        uint8_t start_col = i * char_width;
        map_char_buffer(model, frame_buff, render_state.char_buff[i], start_col, arr_width);
    }

    #if LOG_BUFFER_CONTENT
        //# Print the buffer
        ESP_LOGI(TAG, "Frame Buffer Contents:");

        for (int j = 0; j < font_height; j++) {
            for (int i = 0; i < arr_width; i++) {
                uint16_t value = frame_buff[i + j * arr_width];
                
                if (value < 1) {
                    printf("__");
                } else {
                    printf("##");
                    // printf("%04X ", value);
                }
            }
            printf("\n");
        }
    #endif

    uint8_t x1 = render_state.x0 + (render_state.char_count * model->font_width) + (render_state.char_count - 1) * spacing;
    uint8_t y1 = render_state.current_y + font_height - 1;

    //# Print the buffer details
    printf("\nL%d: %d char, spacing: %u,\t [x=%d, y=%d] to [x=%d, y=%d]",
        render_state.line_idx, render_state.char_count, spacing,
        // start position
        render_state.x0, render_state.current_y, 
        // end position
        x1, y1
    );

    // Print the accumulated characters
    printf("\t\t");
    for (uint8_t i = 0; i < render_state.char_count; i++) {
        printf("%c", render_state.char_buff[i]);
    }

    //! Send the buffer to the display    
    modTFT_setWindow(render_state.x0, render_state.current_y, x1, y1, config);
    send_spi_data((uint8_t *)frame_buff, buff_len * 2, config); // Multiply by 2 for uint16_t size

    // Reset the accumulated character count and update render_start_x
    render_state.char_count = 0;
    render_state.x0 = x1;

    free(frame_buff); // Free the allocated buffer
    frame_buff = NULL; // Good practice to avoid dangling pointers
}

int flush_buffer_and_reset(uint8_t font_height) {
    //! Move to the next line
    render_state.line_idx++;

    //! Reset state for the next line
    render_state.x0 = 0;                                    // Reset render_start_x to the start position
    render_state.current_y += font_height + 1; // Move y to the next line

    //! Check if current_y is out of bounds
    return render_state.current_y + font_height > ST7735_HEIGHT;
}

void st7735_draw_text(M_TFT_Text *model, M_Spi_Conf *config) {
    if (!model || !config || !model->text || !model->font) return; // Error handling

    // Pointer to the text
    const char *text = model->text;
    uint8_t char_width = model->font_width + model->char_spacing;
    uint8_t font_height = model->font_height;

    //# Initialize rendering state
    uint8_t current_x = model->x;

    render_state.current_y = model->y;
    render_state.x0 = model->x;

    while (*text) {
        //# Find the end of the current word and calculate its width
        uint8_t word_width = 0;

        const char *word_end = text;
        while (*word_end && *word_end != ' ' && *word_end != '\n') {
            word_width += char_width;
            word_end++;
        }

        //# Handle newline character
        if (*text == '\n') {
            current_x = 0;

            send_text_buffer(model, config);
            if (flush_buffer_and_reset(font_height)) break;        //! Exit the outerloop if break
            text++;     // Move to the next character
            continue;   // continue next iteration
        }

        if (current_x + word_width > ST7735_WIDTH) {
            //# Handle page wrapping
            if (model->page_wrap == 0) {
                //! Stop printing and move to the next line
                current_x = 0;

                send_text_buffer(model, config);
                if (flush_buffer_and_reset(font_height)) break;

                //! Skip all remaining characters on the same line
                while (*text && *text != '\n') text++;
                continue; // Skip the rest of the loop and start processing the next line
            }
            //# Handle word wrapping
            else if (model->word_wrap) {
                //! Stop printing and move to the next line
                current_x = 0;

                send_text_buffer(model, config);
                if (flush_buffer_and_reset(font_height)) break;
            }
        }

        //# This while loop processes a sequence of characters (stored in the text pointer) until it reaches
        //# the end of a word (word_end) or encounters a space character (' ')
        
        while (text < word_end || (*text == ' ' && text < word_end + 1)) {
            //# Handle word breaking when word_wrap is disabled
            if (!model->word_wrap && current_x + char_width > ST7735_WIDTH) {
                current_x = 0;

                send_text_buffer(model, config);
                if (flush_buffer_and_reset(font_height)) break;
            }

            // Accumulate the current character
            render_state.char_buff[render_state.char_count++] = *text;

            //! Check if the buffer is full
            if (render_state.char_count >= MAX_CHAR_COUNT) {
                send_text_buffer(model, config);
            }

            // Move to the next character position
            current_x += char_width;     // Move x by character width + spacing
            text++;                                                 // Move to the next character
        }
    }

    //! Print the remaining buffer if it has data
    send_text_buffer(model, config);
}
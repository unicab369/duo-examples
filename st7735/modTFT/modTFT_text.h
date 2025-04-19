
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
    spi_send_data((uint8_t *)frame_buff, buff_len * 2, config); // Multiply by 2 for uint16_t size

    // Reset the accumulated character count and update render_start_x
    render_state.char_count = 0;
    render_state.x0 = x1;

    free(frame_buff); // Free the allocated buffer
    frame_buff = NULL; // Good practice to avoid dangling pointers
}

static int flush_buffer_and_reset(uint8_t font_height) {
    //! Move to the next line
    render_state.line_idx++;

    //! Reset state for the next line
    render_state.x0 = 0;                                    // Reset render_start_x to the start position
    render_state.current_y += font_height + 1; // Move y to the next line

    //! Check if current_y is out of bounds
    return render_state.current_y + font_height > ST7735_HEIGHT;
}

void modTFT_drawText(M_TFT_Text *model, M_Spi_Conf *config) {
    if (!model || !config || !model->text || !model->font) return; // Error handling

    // Pointer to the text
    const char *text = model->text;
    uint8_t char_width = model->font_width + model->char_spacing;
    uint8_t font_height = model->font_height;

    //# Initialize rendering state
    uint8_t current_x = model->x;

    render_state.current_y = model->y;
    render_state.x0 = model->x;

    digitalWrite(config->CS, 0);
    
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

    digitalWrite(config->CS, 1);
}
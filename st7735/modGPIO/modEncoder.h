#include <wiringx.h>

#define DEBOUNCE_TIME_MS 1      // Dont make this number too big

typedef struct {
    int DT_PIN;
    int CLK_PIN;
    int direction;  // 1: CW, -1: CCW
    int position;
    int last_state;
    uint32_t last_change_time;
    int print_log;
} M_Encoder;

int get_state(int dtPin, int clkPin) {
    return (digitalRead(clkPin) << 1) | digitalRead(dtPin);
}

void encoder_init(M_Encoder *model) {
    model->position = 0;
    model->last_state = 0;
    model->last_change_time = 0;

    pinMode(model->DT_PIN, PINMODE_INPUT);
    pinMode(model->CLK_PIN, PINMODE_INPUT);
    model->last_state = get_state(model->DT_PIN, model->CLK_PIN);
}

void encoder_task(M_Encoder *model, void (*callback)(int, int)) {
    int new_state = get_state(model->DT_PIN, model->CLK_PIN);
    if (new_state == model->last_state) return;

    // handle debounce
    uint32_t now = millis();
    if ((now - model->last_change_time) < DEBOUNCE_TIME_MS) return;
    model->last_change_time = now;

    // Detect transitions only when CLK changes
    if ((model->last_state & 0b10) != (new_state & 0b10)) {
        // Determine direction
        if ((model->last_state == 0b00 && new_state == 0b10) || 
            (model->last_state == 0b10 && new_state == 0b11) ||
            (model->last_state == 0b11 && new_state == 0b01) ||
            (model->last_state == 0b01 && new_state == 0b00)) {
            
            // Clockwise rotation
            model->direction = 1;
            model->position++;
        }
        else {
            // Counter-clockwise rotation
            model->direction = -1;
            model->position--;
        }

        callback(model->position, model->direction);

        if (model->print_log)
            printf("Position: %d, CW: %d\n", model->position, model->direction);
    }

    model->last_state = new_state;
}

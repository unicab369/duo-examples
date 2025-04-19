#include <wiringx.h>

typedef struct {
    int PIR_PIN;
    int current_state;
    uint32_t last_change_time;
    int print_log;
} M_PIR;

void pir_init(M_PIR *model) {
    pinMode(model->PIR_PIN, PINMODE_INPUT);
}

void pir_task(M_PIR *model, void (*callback)(int)) {
    if (refresh_change_time(&model->last_change_time) < 0) return;

    int read = digitalRead(model->PIR_PIN);
    if (read != model->current_state) {
        callback(read);

        if (model->print_log)
            printf("pir read: %d\n", read);
    }

    model->current_state = read;
}
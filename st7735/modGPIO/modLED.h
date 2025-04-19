#include <wiringx.h>


typedef struct {
    int LED_PIN;
    int current_state;
} M_LED;


void led_init(M_LED *model) {
    pinMode(model->LED_PIN, PINMODE_OUTPUT);
}

void led_set(M_LED *model, int state) {
    digitalWrite(model->LED_PIN, state);
}

void led_on(M_LED *model) {
    digitalWrite(model->LED_PIN, 1);
}

void led_off(M_LED *model) {
    digitalWrite(model->LED_PIN, 0);
}

void led_toggle(M_LED *model) {
    digitalWrite(model->LED_PIN, !digitalRead(model->LED_PIN));
}
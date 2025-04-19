//! stolen from: https://github.com/taspenwall/PWM-test/

#include <stdio.h>
#include "wiringx.h"
#include <signal.h>


#define PIN_NUM 2 //GPIO 2 
int num_leds = 300;

typedef struct {
    uint8_t GREEN;
    uint8_t RED;
    uint8_t BLUE;
} M_RGB;


#include "devmap.h"

struct pinlist {
	char name[32];
	uint32_t offset;
};

struct pinlist cv180x_pin[] = {
	{ "GP0", 0x4c },				// IIC0_SCL
	{ "GP1", 0x50 },				// IIC0_SDA
	{ "GP2", 0x84 },				// SD1_GPIO1
	{ "GP3", 0x88 },				// SD1_GPIO0
	{ "GP4", 0x90 },				// SD1_D2
	{ "GP5", 0x94 },				// SD1_D1
	{ "GP6", 0xa0 },				// SD1_CLK
	{ "GP7", 0x9c },				// SD1_CMD
	{ "GP8", 0x98 },				// SD1_D0
	{ "GP9", 0x8c },				// SD1_D3
	{ "GP10", 0xf0 },				// PAD_MIPIRX1P
	{ "GP11", 0xf4 },				// PAD_MIPIRX0N
	{ "GP12", 0x24 },				// UART0_TX
	{ "GP13", 0x28 },				// UART0_RX
	{ "GP14", 0x1c },				// SD0_PWR_EN
	{ "GP15", 0x20 },				// SPK_EN
	{ "GP16", 0x3c },				// SPINOR_MISO
	{ "GP17", 0x40 },				// SPINOR_CS_X
	{ "GP18", 0x30 },				// SPINOR_SCK
	{ "GP19", 0x34 },				// SPINOR_MOSI
	{ "GP20", 0x38 },				// SPINOR_WP_X
	{ "GP21", 0x2c },				// SPINOR_HOLD_X
	{ "GP22", 0x68 },				// PWR_SEQ2
	{ "GP26", 0xa8 },				// ADC1
	{ "GP27", 0xac },				// USB_VBUS_DET

	{ "GP25", 0x12c },				// PAD_AUD_AOUTR
};

#define PIN_MUX_BASE  0x03001000

int get_func(int pin) {
    devmap_open(PIN_MUX_BASE + cv180x_pin[pin].offset, 4);
	uint32_t func = devmap_readl(0);
	devmap_close(4);
	return func;

}

void set_func(int pin, uint32_t set_func) {
	devmap_open(PIN_MUX_BASE + cv180x_pin[pin].offset, 4);
	devmap_writel(0, set_func);
	uint32_t func = devmap_readl(0);
	devmap_close(4);
}

//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

void handle_sigint(int sig) { 
    pwm_cleanup();
    printf("Caught signal now %d\n", sig);
    exit(0);
} 

void shiftSetColor(
    M_RGB *strand, const M_RGB *static_colors,
    int *currentColorIndex, int num_colors
) {
    for (int i = num_leds - 1; i >=5 ; i--){
        strand[i] = strand[i-5]; // shift down the row
    }

    for (int j=0; j<5; j++){ // make the 1st 5 leds the current color
        strand[j] = static_colors[*currentColorIndex];
    }

    //strand[0] = static_colors[*currentColorIndex]; // set the first led to the current color
    *currentColorIndex = ((*currentColorIndex + 1) % num_colors); // increment the color index
}


int test_led() {
    signal(SIGINT, handle_sigint);
    set_func(PIN_NUM, 0x7); //if you change the pin number you need to lookup the function number of the PWM pin in the excel sheet.
    setup_pwm();
    
    const M_RGB static_colors[7] = {
        {0, 255, 0}, //red
        {65, 255, 0},//orange
        {255, 255, 0}, //yellow
        {255, 0, 0}, //green
        {150, 0, 150}, //indigo
        {0, 0, 255}, //blue 
        {0, 255, 100}, //violet        
    };
   
    M_RGB strand[num_leds];
    int currentColorIndex = 0;

    while (1) {
        // shift the colors down the strand, and set the first led to the next color
        int num_colors = sizeof(static_colors)/sizeof(static_colors[0]);
        shiftSetColor(strand, static_colors, &currentColorIndex, num_colors);
        
        for (int i = 0; i < num_leds; i++){
            address_led(strand[i].GREEN, strand[i].RED, strand[i].BLUE); //send to the leds
        }

       delayMicroseconds(500000); 
    }
    
    return 0;
}

//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
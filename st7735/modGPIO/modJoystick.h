#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

char adc_chan;

typedef struct {
    int device;
    int X_PIN;
    int Y_PIN;
    uint32_t last_change_time;
    void (*callback)(int, int);
} M_Joystick;

int joystick_init(M_Joystick *model) {
	if ((system("lsmod | grep -q \"cv180x_saradc\"") == 0) ||
        (system("lsmod | grep -q \"cv181x_saradc\"") == 0)) {
		    printf("SARADC module already loaded.\n");
	}else{
		system("insmod $(find / -name \"cv180x_saradc.ko\" 2>/dev/null)");
		system("insmod $(find / -name \"cv181x_saradc.ko\" 2>/dev/null)");
		printf("SARADC module loaded.\n");
	}
	
    model->device = open("/sys/class/cvi-saradc/cvi-saradc0/device/cv_saradc", O_RDWR|O_NOCTTY|O_NDELAY);
	if (model->device < 0){
		printf("Error at opening ADC!\n");
		return -1;
	}

    return 1;
}

void joystick_task(M_Joystick *model, void (*callback)(int, int)) {
    uint32_t now = millis();
    if (now - model->last_change_time < 200) return;
    model->last_change_time = now;

    char buffer[32];

    adc_chan = '1';
    write(model->device, &adc_chan, 1);
    lseek(model->device, 0, SEEK_SET);
    read(model->device, buffer, sizeof(buffer));
    int x_value = atoi(buffer);

    adc_chan = '2';
    write(model->device, &adc_chan, 1);
    lseek(model->device, 0, SEEK_SET);
    read(model->device, buffer, sizeof(buffer));
    int y_value = atoi(buffer);
    
    callback(x_value, y_value);
}

// close(fd);

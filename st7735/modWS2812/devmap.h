//! stolen from: https://github.com/taspenwall/PWM-test/


#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>

void *mapped_base;
void *virtmem;
int devmap_fd;      // file descriptor

void *devmap(int addr, int len) {
    devmap_fd = open("/dev/mem", O_RDWR | O_SYNC);
    if (devmap_fd == -1) {
        printf("open failed\n");
        return NULL;
    }
    
    off_t offset = addr & ~(sysconf(_SC_PAGE_SIZE) - 1);
    mapped_base = mmap(NULL, len, PROT_READ | PROT_WRITE, MAP_SHARED, devmap_fd, offset);
   
    if (mapped_base == MAP_FAILED) {
        printf("mmap failed\n");
        close(devmap_fd);
        return NULL;
    }
    return mapped_base + (addr - offset);
}

void devmap_open(int addr, int len) {    
    virtmem = devmap(addr, len);

    if (virtmem == NULL) {
        printf("devmap failed\n");
    }
}

uint32_t devmap_readl(int offset) {
    if(virtmem == NULL) {
        printf("devmap not open for read\n");
        return 0;
    }
        
    return *(volatile uint32_t *)(virtmem + offset);
}

void devmap_writel(int offset, uint32_t value) {
    if(mapped_base == NULL) {
        printf("devmap not open for write\n");
        return;
    }

    *(volatile uint32_t *)(virtmem + offset) = value;
}

void devmap_unmap(void *virt_addr, int len) {
    if(mapped_base != NULL) {
         //printf("closing with munmap\n");
         munmap (virt_addr, len);
         close(devmap_fd);
    }
}

void devmap_close(int len) {
    if(mapped_base != NULL) {
        devmap_unmap(mapped_base, len);
        mapped_base = NULL;
    }
}

//! PWM
#define PWM_BASE    0x03060000
#define PWM_GROUP   0x2000
#define HLPERIDO2   0x010

#define PWM_START   0x044
#define PWM_DONE    0x048
#define PWM_UPDATE  0x04C
#define PWM_MODE    0x040
#define PWM_OE      0x0D0
#define PERIOD2     0x014
#define PCOUNT2     0x058

#define pwm_mode (1<<10) // set the pmode to to pulse 1<<num+8 and set the polarity to control the high period 1<<2

// 100 is the calcualted for 800kHz calulated frequency, 
// It seem that I have to go way lower for a 1.25us period.
#define freq    100   
#define Hduty   65  //65 is the claculated duty cycle for .7us high time to write a 1
#define Lduty   10  //10 %  is the claculated duty cycle for .35us high time   to write a 0


void setup_pwm() {
    devmap_open(PWM_BASE + PWM_GROUP, 1024);
   //set the pwm registers
   devmap_writel(PWM_OE, 0b0100); //output enable
   
   devmap_writel(PWM_MODE, pwm_mode);   // set count mode and polarity
   devmap_writel(PWM_START, 0);         // set start to 0
   devmap_writel(PERIOD2, freq);        //set PERIOD2 to 800kHz
   devmap_writel(PCOUNT2, 1);           //set PCOUNT2 for 1 pulse
}

void send_bin (uint8_t num){
    for (int i = 0; i < 8; i++){
        devmap_writel(PWM_START, 0); //clear the start bit
   
        int duty = (num >> i & 1 == 1) ? Lduty : Hduty;
        devmap_writel(0x010, duty*freq/100);        // set helper period HLPERIDO2
     
        // kick and wait
        devmap_writel(PWM_START, 0b0100);
        while(devmap_readl(PWM_DONE) == 0);         // wait for done 
    }    
 }

void address_led(uint8_t green, uint8_t red, uint8_t blue){
   send_bin(green);
   send_bin(red);
   send_bin(blue);
}

void pwm_cleanup(){
   devmap_writel(PWM_START, 0);
   devmap_writel(PWM_OE, 0);
   devmap_close(1024);
}
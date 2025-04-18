#include "modSSD1306_draw.h"

#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>

int host_ssd1306_0 = -1;
int host_ssd1306_1 = -1;
int host_bh1750 = -1;
int host_sht31 = -1;
int host_ap3216 = -1;
int host_apds9960 = -1;
int host_max44009 = -1;
int host_vl53lox = -1;
int host_mpu6050 = -1;
int host_ina219 = -1;
int host_ds3231 = -1;
int host_ds1307 = -1;

int modI2C_setupDevice(int channel, int address, int *host_id) {
    char filename[32];
    sprintf(filename,"/dev/i2c-%d", channel);

    if ((*host_id = wiringXI2CSetup(filename, address)) <0) {
        printf("\nI2C Setup failed.\n");
        return -1;
    }

    printf("\nconnected to host: %u\n", *host_id);
    return 1;
}

typedef struct {
    int sec, min, hr;
    int day;        // 1: Monday to 7: Sunday
    int date, month, year;
} M_DateTime;


int ssd3231_update_time(int host, M_DateTime *dt) {
    int ret = wiringXI2CWriteReg8(host, 0x00, dt->sec);
    ret = wiringXI2CWriteReg8(host, 0x01, dt->min);
    ret = wiringXI2CWriteReg8(host, 0x02, dt->hr);
    return ret;
}

int ssd3231_update_date(int host, M_DateTime *dt) {
    int ret = wiringXI2CWriteReg8(host, 0x04, DECIMAL_TO_BCD(dt->date));
    ret = wiringXI2CWriteReg8(host, 0x05, DECIMAL_TO_BCD(dt->month));
    ret = wiringXI2CWriteReg8(host, 0x06, DECIMAL_TO_BCD(dt->year-2000));
    return ret;
}

int modI2C_init() {
    int ret;

    modI2C_setupDevice(0, 0x3C, &host_ssd1306_0);
    ssd1306_init(host_ssd1306_0);

    modI2C_setupDevice(1, 0x3C, &host_ssd1306_1);
    ssd1306_init(host_ssd1306_1);

    //! BH1750
    modI2C_setupDevice(0, 0x23, &host_bh1750);
    // BH1750 POWER_ON = 0x01, POWER_DOWN = 0x00
    ret = write(host_bh1750, (uint8_t[]){ 0x01 }, 1);
    if (ret < 0) printf("host_bh1750 init failed.\n");

    //! SHT31
    modI2C_setupDevice(0, 0x40, &host_sht31);
    // SOFT_RESET 0x30, 0xA2
    ret = write(host_sht31, (uint8_t[]){ 0x30, 0xA2 }, 2);
    if (ret < 0) printf("host_sht31 init failed.\n");

    //! AP3216
    modI2C_setupDevice(0, 0x1E, &host_ap3216);
    // SOFT_RESET 0x00, 0x03
    ret = write(host_ap3216, (uint8_t[]){ 0x00, 0x03 }, 2);
    if (ret < 0) printf("host_ap3216 init failed.\n");

    //! APDS9960
    modI2C_setupDevice(0, 0x39, &host_apds9960);

    uint8_t apds9960_config[] = {
        0x80,           // ENABLE
        0x0F,           // Power ON, Proximity enable, ALS enable, Wait enable
        0x90,           // CONFIG2
        0x01,           // Proximity gain control (4x)
        0x8F, 0x20,     // Proximity pulse count (8 pulses)
        0x8E, 0x87      // Proximity pulse length (16us)
    };

    ret = write(host_apds9960, apds9960_config, sizeof(apds9960_config));
    if (ret < 0) printf("host_apds9960 init failed.\n");

    //! MAX44009
    modI2C_setupDevice(0, 0x40, &host_max44009);
    // CONFIG_REG 0x02, CONFIG_VALUE 0x80
    ret = wiringXI2CWriteReg8(host_max44009, 0x02, 0x80);
    if (ret < 0) printf("host_max44009 init failed.\n");

    //! VL53LOX
    modI2C_setupDevice(0, 0x29, &host_vl53lox);
    ret = wiringXI2CWriteReg8(host_vl53lox, 0x00, 0x01);

    // modI2C_setupDevice2(0, 0x29, &host_vl53lox);
    // START RANGING REG 0x00, 0x01
    // ret = write(host_vl53lox, (uint8_t[]){ 0x00, 0x01 }, 2);
    if (ret < 0) printf("\nhost_vl53lox init failed.\n");

    //! MPU6050
    modI2C_setupDevice(0, 0x68, &host_mpu6050);
    // WAKEUP REG 0x6B, 0x00
    ret = wiringXI2CWriteReg8(host_mpu6050, 0x6B, 0x00);
    if (ret < 0) printf("host_mpu6050 init failed.\n");

    //! INA219
    modI2C_setupDevice(0, 0x40, &host_ina219);  // adjust A0 and A1 for 0x41, 0x44, 0x45
    // CONFIG REG 0x00
    ret = wiringXI2CWriteReg16(host_ina219, 0x00, 0x399F);
    if (ret < 0) printf("host_ina219 init failed.\n");

    //! DS3231
    modI2C_setupDevice(0, 0x68, &host_ds3231);

    // CONFIG REG 0x00
    ret = wiringXI2CWrite(host_ds3231, 0x00);
    if (ret > 0) {
        M_DateTime dateTime = {
            .sec = 31, .min = 30, .hr = 4,
            .date = 16, .month = 4, .year = 2025
        };

        printf("host_ds3231 init success.\n");
        ret = ssd3231_update_time(host_ds3231, &dateTime);
        ret = ssd3231_update_date(host_ds3231, &dateTime);
    } else {
        printf("host_ds3231 init failed.\n");
    }

    return 1;
}


//! SSD3231
void ssd3231_read_dateTime(int host, M_DateTime *dt) {
    uint8_t raw[7];
    read(host, raw, 7);

    dt->sec     = BCD_TO_DECIMAL(raw[0]);
    dt->min     = BCD_TO_DECIMAL(raw[1]);
    dt->hr      = BCD_TO_DECIMAL(raw[2]);
    dt->day     = BCD_TO_DECIMAL(raw[3]);
    dt->date    = BCD_TO_DECIMAL(raw[4]);
    dt->month   = BCD_TO_DECIMAL(raw[5] & 0x1F);              // Mask out the century bit
    dt->year    = BCD_TO_DECIMAL(raw[6]) + 2000;
}

typedef enum {
    BH1750_CONTINUOUS_1LX       = 0x10,     // 1 lux resolution 120ms
    BH1750_CONTINUOUS_HALFLX    = 0x11,     // .5 lux resolution 120ms
    BH1750_CONTINUOUS_4LX_RES   = 0x13,     // 4 lux resolution 16ms
    BH1750_ONETIME_1LX          = 0x20,     // 1 lux resolution 120ms
    BH1750_ONETIME_HALFLX       = 0x21,     // .5 lux resolution 120ms
    BH1750_ONETIME_4LX          = 0x23,     // 4 lux resolution 16ms

} E_BH1750_MODE;

//! BH1750
void bh1750_get_reading(int host) {
    // wiringXI2CWrite(host, BH1750_CONTINUOUS_4LX_RES);
    int reading = wiringXI2CReadReg16(host, BH1750_CONTINUOUS_4LX_RES);
    // int reading = wiringXI2CRead(host);
    printf("BH1750 reading: %d\n", reading);
}

//! AP3216
void ap3216_get_reading(int host) {
    int ps = wiringXI2CReadReg16(host, 0x0F);
    int als = wiringXI2CReadReg16(host, 0x0D);
    printf("AP3216 ps: %d, als: %d\n", ps, als);
}

//! AP9960
void ap9960_get_reading(int host) {
    int prox = wiringXI2CReadReg8(host, 0x9C);

    uint8_t raw[8];
    read(host, raw, 8);

    int clear  = (raw[1] << 8) | raw[0];
    int red    = (raw[3] << 8) | raw[2];
    int green  = (raw[5] << 8) | raw[4];
    int blue   = (raw[7] << 8) | raw[6];

    printf("AP9960 prox: %d, clear: %d, red: %d, green: %d, blue: %d\n",
            prox, clear, red, green, blue);
}

//! MAX44009
void max44009_get_reading(int host) {
    int reading = wiringXI2CReadReg16(host, 0x03);
    printf("MAX44009 reading: %d\n", reading);
}

//! VL53L0X
void vl53l0x_get_reading(int host) {
    wiringXI2CWrite(host, 0x13);    // Request Status

    int reading = wiringXI2CReadReg16(host, 0x14);
    printf("VL53L0X reading: %d\n", reading);
}

//! MPU6050
void mpu6050_get_reading(int host) {
    uint8_t raw[6];
    read(host, raw, 6);
    int accel_x = (raw[0] << 8) | raw[1];
    int accel_y = (raw[2] << 8) | raw[3];
    int accel_z = (raw[4] << 8) | raw[5];
    printf("MPU6050 accel: %d, %d, %d\n", accel_x, accel_y, accel_z);
}

//! INA219
void ina219_get_reading(int host) {
    int shunt   = wiringXI2CReadReg16(host, 0x01);
    int bus     = wiringXI2CReadReg16(host, 0x02);
    int power   = wiringXI2CReadReg16(host, 0x03);
    int current = wiringXI2CReadReg16(host, 0x04);
    printf("INA219 shunt: %d, bus: %d, power: %d, current: %d\n",
            shunt, bus, power, current);
}

//! SHT31
void sht31_get_reading(int host) {
    int ret = wiringXI2CWriteReg8(host, 0x24, 0x00);    // NOHEAT 0x24
    if (ret < 0) {
        printf("SHT31 error: %d\n", ret);
        return;
    }

    // int reading = wiringXI2CReadReg16(host, 0x2400);    // HIGHREP 0x2400
    // printf("SHT31 reading: %d\n", reading);
}

static uint32_t last_i2c_time = 0;

void modI2C_task(int print_log) {
    uint32_t now = millis();
    if (now - last_i2c_time < 200) return;
    last_i2c_time = now;

    test_ssd1306_draw(0, host_ssd1306_0);
    // test_ssd1306_draw(0, host_ssd1306_1);

    // bh1750_get_reading(host_bh1750);
    // ap3216_get_reading(host_ap3216);
    // ap9960_get_reading(host_apds9960);
    // max44009_get_reading(host_max44009);
    // vl53l0x_get_reading(host_vl53lox);
    // mpu6050_get_reading(host_mpu6050);
    // ina219_get_reading(host_ina219);
    // sht31_get_reading(host_sht31);

    // M_DateTime dt;
    // ssd3231_read_dateTime(host_ds3231, &dt);
    // printf("DS3231 reading: %d:%d:%d %d/%d/%d\n", dt.hr, dt.min, dt.sec, dt.date, dt.month, dt.year);
    // printf("\n");
}
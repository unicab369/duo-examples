#include <stdio.h>
#include <unistd.h>
#include <stdint.h>  

#include <wiringx.h>

#define SPI_NUM     2
#define SPI_SPEED   10E6

int SPI_RST = 4;
int SPI_DC = 5;
int CS_PIN = 15;

typedef struct {
    uint8_t host;
    int8_t mosi;
    int8_t miso;
    int8_t clk;
    int8_t cs;      //! the main cs pin

    //! dc and rst are not part of SPI interface
    //! They are used for some display modules
    int8_t dc;          // set to -1 when not use
    int8_t rst;         // set to -1 when not use
} M_Spi_Conf;

void mod_spi_cmd(uint8_t cmd, M_Spi_Conf *conf) {
    digitalWrite(CS_PIN, LOW);
    digitalWrite(SPI_DC, LOW);  // Command mode
    wiringXSPIDataRW(SPI_NUM, &cmd, 1);
    digitalWrite(CS_PIN, HIGH);
}

int mod_spi_data(uint8_t *data, int len, M_Spi_Conf *conf) {
    digitalWrite(CS_PIN, LOW);
    digitalWrite(SPI_DC, HIGH);  // Data mode
    wiringXSPIDataRW(SPI_NUM, data, len);
    digitalWrite(CS_PIN, HIGH);
    return 1;
}

int mod_spi_init(M_Spi_Conf *conf, uint32_t speed) {
    int fd_spi;

    if ((fd_spi = wiringXSPISetup(SPI_NUM, speed)) <0) {
        printf("SPI Setup failed: %i\n", fd_spi);
        return -1;
    }

    if (conf->cs > 0) pinMode(conf->cs, PINMODE_OUTPUT);
    if (conf->dc > 0) pinMode(conf->dc, PINMODE_OUTPUT);

    if (conf->rst > 0) {
        pinMode(conf->rst, PINMODE_OUTPUT);
        digitalWrite(conf->rst, HIGH);

        digitalWrite(conf->rst, LOW);
        delayMicroseconds(10);
        digitalWrite(conf->rst, HIGH);
    }
}
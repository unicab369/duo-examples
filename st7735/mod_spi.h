#include <stdio.h>
#include <unistd.h>
#include <stdint.h>  

#include <wiringx.h>


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
    digitalWrite(conf->cs, LOW);
    digitalWrite(conf->dc, LOW);  // Command mode
    wiringXSPIDataRW(SPI_NUM, &cmd, 1);
    digitalWrite(conf->cs, HIGH);
}

int mod_spi_data(uint8_t *data, int len, M_Spi_Conf *conf) {
    digitalWrite(conf->cs, LOW);
    digitalWrite(conf->dc, HIGH);  // Data mode
    wiringXSPIDataRW(SPI_NUM, data, len);
    digitalWrite(conf->cs, HIGH);
    return 1;
}

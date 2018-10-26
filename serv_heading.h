#include "ch.h"
#include "chprintf.h"
#include "hal.h"
#include "lis302dl.h"

#include <math.h>

#ifndef _SERV_HEAD_H
#define _SERV_HEAD_H

int magSetup(I2CDriver *i2cp, ioportid_t gpio, int sda, int sdl, uint8_t DMA);
int magSetRegisters(uint8_t reg1, uint8_t reg2, uint8_t reg3);
int magGetRegValues(void);
int memsGetRegValues(void);
int getHeading(void);
float heading_calculate(void);
int16_t complement2signed(uint8_t msb, uint8_t lsb);
int memsSetup(SPIDriver *spi, uint8_t DMA);
int memsSetRegister(uint8_t reg1, uint8_t reg2, uint8_t reg3);
int headingSetup(void);
#endif

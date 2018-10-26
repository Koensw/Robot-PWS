#include "serv_heading.h"
#define mag_addres 0x1E  // magenetomter i2caddres
#define MAG_DATA_REG 0x3 // addres of data
#define MAG_RX_DEPTH 6   // buffer depth for RX data
#define MAG_TX_DEPTH 6   // TX buffer 3 control registr + information

static const I2CConfig i2cfg2 = {
    OPMODE_I2C,
    400000,
    FAST_DUTY_CYCLE_2,
};
static const SPIConfig spi1cfg = {NULL,
                                  /* HW dependent part.*/
                                  GPIOE, GPIOE_CS_SPI,
                                  SPI_CR1_BR_0 | SPI_CR1_BR_1 | SPI_CR1_CPOL |
                                      SPI_CR1_CPHA};

msg_t status = RDY_OK;
static uint8_t rxbuf[6];
static uint8_t txbuf[6];
static I2CDriver *driver;
static SPIDriver *sdriver;

int X_m;
int Y_m;
int Z_m;
int X_a;
int Y_a;
int Z_a;
float heading;

int headingSetup() {
  memsSetup(&SPID1, FALSE);
  magSetup(&I2CD2, GPIOB, 10, 11, 0);
  if (magSetRegisters(0x70, 0xa0, 0x00) == -1) {
  }
  memsSetRegister(0x43, 0x0, 0x0);
  return 0;
}

int memsSetup(SPIDriver *spi, uint8_t DMA) {
  if (!DMA) {
    spiStart(spi, &spi1cfg); // start spi
    sdriver = spi;
  }
  return 0;
}

int magSetup(I2CDriver *i2cp, ioportid_t gpio, int sda, int sdl, uint8_t DMA) {
  if (!DMA) {
    i2cStart(i2cp, &i2cfg2);
    driver = i2cp;
    palSetPadMode(gpio, sda, PAL_MODE_ALTERNATE(4));
    palSetPadMode(gpio, sdl, PAL_MODE_ALTERNATE(4));
  }
  return 0;
}
int magSetRegisters(uint8_t reg1, uint8_t reg2, uint8_t reg3) {
  status = RDY_OK;
  systime_t tmo = MS2ST(4);
  txbuf[0] = 0x00;
  txbuf[1] = reg1;
  txbuf[2] = 0x01;
  txbuf[3] = reg2;
  txbuf[4] = 0x02;
  txbuf[5] = reg3;
  i2cAcquireBus(driver);
  status =
      i2cMasterTransmitTimeout(driver, mag_addres, txbuf, 6, rxbuf, 0, tmo);
  i2cReleaseBus(driver);
  if (status != RDY_OK) {
    return -1;
  }
  return 0;
}
int memsSetRegister(uint8_t reg1, uint8_t reg2, uint8_t reg3) {
  lis302dlWriteRegister(sdriver, LIS302DL_CTRL_REG1, reg1);
  lis302dlWriteRegister(sdriver, LIS302DL_CTRL_REG2, reg2);
  lis302dlWriteRegister(sdriver, LIS302DL_CTRL_REG3, reg3);
  return 0;
}

int magGetRegValues() {
  status = RDY_OK;
  systime_t tmo = MS2ST(4);
  static uint8_t rxbuf[6];
  static uint8_t txbuf[6];
  txbuf[0] = 3;
  i2cAcquireBus(driver);
  status =
      i2cMasterTransmitTimeout(driver, mag_addres, txbuf, 1, rxbuf, 6, tmo);
  i2cReleaseBus(driver);
  if (status != RDY_OK) {
  }
  X_m = complement2signed(rxbuf[0], rxbuf[1]);
  Y_m = complement2signed(rxbuf[4], rxbuf[5]);
  Z_m = complement2signed(rxbuf[2], rxbuf[3]);
  return 0;
}
int memsGetRegValues() {
  X_a = (int8_t)lis302dlReadRegister(&SPID1, LIS302DL_OUTX);
  Y_a = (int8_t)lis302dlReadRegister(&SPID1, LIS302DL_OUTY);
  Z_a = (int8_t)lis302dlReadRegister(&SPID1, LIS302DL_OUTZ);
  return 0;
}
int getHeading() {
  magGetRegValues();
  memsGetRegValues();
  int value = (int)heading_calculate();
  if (value < 0) {
    value += 360;
  }
  return X_a;
}
float heading_calculate() {
  float rollRadians = asin(Y_a);
  float pitchRadians = asin(X_a);
  float cosPitch = cos(pitchRadians);
  float sinPitch = sin(pitchRadians);
  float cosRoll = cos(rollRadians);
  float sinRoll = sin(rollRadians);
  float Xh = X_m * cosPitch + Z_m * sinPitch;
  float Yh =
      X_m * sinRoll * sinPitch + Y_m * cosRoll - Z_m * sinRoll * cosPitch;
  float heading = atan2(Yh, Xh);
  heading = heading;
  return heading;
}
int16_t complement2signed(uint8_t msb, uint8_t lsb) {
  uint16_t word = 0;
  word = (msb << 8) + lsb;
  if (msb > 0x7F) {
    return -1 * ((int16_t)((~word) + 1));
  }
  return (int16_t)word;
}
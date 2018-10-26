#include "ch.h"
#include "hal.h"
#include <serv_motor.h>
#include <serv_sonar.h>
#include <serv_wifly.h>

#define ADC_CHANNELS 1
#define ADC_BUFFER_DEPTH 8
#define STOP 40

int sonar_en = FALSE;
static adcsample_t sampels[ADC_CHANNELS * ADC_BUFFER_DEPTH];
static int Tdistance = 0;
static char string[7];

char *itoa(int getal) {
  int first = getal / 1000;
  int second = (getal % 1000) / 100;
  int third = (getal % 100) / 10;
  int forth = (getal % 10);
  string[0] = first + 48;
  string[1] = second + 48;
  string[2] = third + 48;
  string[3] = forth + 48;
  string[4] = '\r';
  string[5] = '\n';
  string[6] = '\0';
  return string;
}

static void adccallback(ADCDriver *adcp, adcsample_t *buffer, size_t n) {
  (void)adcp;
  int correction = 0;
  size_t i;
  for (i = 0; i < n; ++i) {
    correction += buffer[i];
  }
  int total = correction / ADC_BUFFER_DEPTH;
  Tdistance = total;
  if (total < STOP) {

    palSetPad(GPIOD, GPIOD_LED3);
  } else {
    palClearPad(GPIOD, GPIOD_LED3);
  }
}
static void adcerrorcallback(ADCDriver *adcp, adcerror_t err) {

  (void)adcp;
  (void)err;
  wiflySend(itoa(err));
}
static const ADCConversionGroup adcgrpcfg2 = {TRUE,
                                              ADC_CHANNELS,
                                              adccallback,
                                              adcerrorcallback,
                                              0,               /* CR1 */
                                              ADC_CR2_SWSTART, /* CR2 */
                                              ADC_SMPR1_SMP_AN14(ADC_SAMPLE_56),
                                              0, /* SMPR2 */
                                              ADC_SQR1_NUM_CH(ADC_CHANNELS),
                                              0, /* SQR2 */
                                              ADC_SQR3_SQ1_N(ADC_CHANNEL_IN14)};
void sonarInit() {
  adcStart(&ADCD1, 0);
  palSetPadMode(GPIOC, 4, PAL_MODE_INPUT_ANALOG);
  palSetPadMode(GPIOD, GPIOD_LED3, PAL_MODE_OUTPUT_PUSHPULL);
}
void sonarStart() {
  sonar_en = TRUE;
  adcStartConversionI(&ADCD1, &adcgrpcfg2, sampels, ADC_BUFFER_DEPTH);
}
void sonarStop() {
  sonar_en = FALSE;
  adcStopConversionI(&ADCD1);
}
int sonarDistance() { return Tdistance; }
int sonarActive() { return sonar_en; }
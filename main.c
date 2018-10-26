#include <math.h> //fpu is enabled
#include <stdlib.h>
#include <string.h>

#include "ch.h"
#include "hal.h"

#include "chprintf.h"
#include "lis302dl.h"

#include "serv_heading.h"
#include "serv_motor.h"
#include "serv_sonar.h"
#include "serv_usbcfg.h"
#include "serv_wifly.h"

extern char *panic_msg;

Message callback(char id[10], char args[1024]) {
  (void)args;

  if (strcmp(id, "version") == 0) {
    // send version (DEBUGGING)
    wiflySend("CHIBIOS-STM32DISCOVERY v0.1\r\n");
    return M_MESSAGE;
  } else if (strcmp(id, "light") == 0) {
    // enable lights (DEBUGGING)
    int ck = atoi(args);
    if (ck == 1)
      palTogglePad(GPIOD, GPIOD_LED4);
    else if (ck == 2)
      palTogglePad(GPIOD, GPIOD_LED5);
    else
      return M_ERROR;
    return M_OK;
  } else if (strcmp(id, "forward") == 0) {
    // motor forward
    int sec = atoi(args);
    if (sec == 0 || motorActive())
      return M_ERROR;
    motorForwardS(sec * 1000);
    return M_OK;
  } else if (strcmp(id, "backward") == 0) {
    // motor backward
    int sec = atoi(args);
    if (sec == 0 || motorActive())
      return M_ERROR;
    motorBackwardS(sec * 1000);
    return M_OK;
  } else if (strcmp(id, "turnLeft") == 0) {
    // motor forward
    int sec = atoi(args);
    if (sec == 0 || motorActive())
      return M_ERROR;
    motorLeftForwardS(sec * 1000);
    return M_OK;
  } else if (strcmp(id, "turnRight") == 0) {
    // motor forward
    int sec = atoi(args);
    if (sec == 0 || motorActive())
      return M_ERROR;
    motorRightForwardS(sec * 1000);
    return M_OK;
  } else if (strcmp(id, "close") == 0) {
    // close session
    wiflySendCommand("close", "AOK");
    return M_OK;
  } else if (strcmp(id, "sonar") == 0) {
    int on = atoi(args);
    if (on == 1) {
      sonarStart();
      return M_OK;
    } else if (on == 0) {
      sonarStop();
      return M_OK;
    }
    return M_OK;
  } else if (strcmp(id, "readHead") == 0) {
    wiflySend(itoa(getHeading()));
    return M_MESSAGE;
  } else if (strcmp(id, "readSonar") == 0) {
    wiflySend(itoa(sonarDistance()));
    return M_MESSAGE;
  } else
    return M_ERROR;
}

int main(void) {

  /*
   * System initializations.
   * - HAL initialization, this also initializes the configured device drivers
   *   and performs the board-specific initializations.
   * - Kernel initialization, the main() function becomes a thread and the
   *   RTOS is active.
   */
  halInit();
  chSysInit();
  /*
   * User initializations
   */
  wiflyInit(&SD2, GPIOA, 2, 3, 115200, callback);
  motorInit(GPIOC, 6, 7, 8, 9, &PWMD3, 1, 0, 3, 2);

  sonarInit();
  headingSetup();

  // CONNECT TO WIFLY HERE

  while (TRUE) {
    chThdSleepMilliseconds(1000);
  }
}

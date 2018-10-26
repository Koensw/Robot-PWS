/*
 * PWS-Robot
 * (c) 2012 Koen Wolters en Erik Kooistra
 *
 * Regelt de communicatie met de motoren
 *
 * Gebruikt driver D2
 */

#ifndef _SERV_MOTOR_H
#define _SERV_MOTOR_H

#include "ch.h"
#include "hal.h"

typedef enum { D_FORWARD, D_BACKWARD } Direction;

/* PUBLIC METHODS */
int motorInit(ioportid_t gpio, int p11, int p12, int p21, int p22,
              PWMDriver *pwm, int lpa, int lpb, int rpa, int rpb);

/*
 * Left forward and backward
 */
void motorLeftForwardS(int);
void motorLeftBackwardS(int);
/*
 * Right forward and backward
 */
void motorRightForwardS(int);
void motorRightBackwardS(int);
/*
 * Two motors
 */
void motorForwardS(int);
void motorBackwardS(int);
/*
 * Stop motor
 */
void motorLeftOff(void);
// void motorLeftBreak();
void motorRightOff(void);
// void motorRightBreak();
void motorOff(void);

/* Motors active */
int motorLeftActive(void);
int motorRightActive(void);
int motorActive(void);
/*
 * Set motor speed
 */
int motorSetSpeed(int s);

/* PRIVATE METHODS */
void motor_left_on(Direction dir);
void motor_right_on(Direction dir);
void motor_left_off(void *p);
void motor_right_off(void *p);

#endif

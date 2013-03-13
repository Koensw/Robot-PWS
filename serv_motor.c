#include "serv_motor.h" 

static PWMConfig motor_pwmcfg = {
10000, /* 10Khz PWM clock frequency */
20, /* PWM period 1 second */
NULL,  /* No callback */
/* Only channel 4 enabled */
{
	{PWM_OUTPUT_ACTIVE_HIGH, NULL},
	{PWM_OUTPUT_ACTIVE_HIGH, NULL},
	{PWM_OUTPUT_ACTIVE_HIGH, NULL},
	{PWM_OUTPUT_ACTIVE_HIGH, NULL},
},
0
};

int lp1;
int lp2;
int rp1;
int rp2;

int motor_speed = 6250;

VirtualTimer vtl;
VirtualTimer vtr;

BinarySemaphore motor_left_sem;
BinarySemaphore motor_right_sem;

PWMDriver *motor_pwm;

//initialize motors
int motorInit(ioportid_t gpio, int p11, int p12, int p21, int p22, PWMDriver *pwm, int lpa, int lpb, int rpa, int rpb){
	//vars
	lp1 = lpa;
	lp2 = lpb;
	rp1 = rpa;
	rp2 = rpb;
	motor_pwm = pwm;
	//configure
	palSetPadMode(gpio, p11, PAL_MODE_ALTERNATE(2));
	palSetPadMode(gpio, p12, PAL_MODE_ALTERNATE(2));
	palSetPadMode(gpio, p21, PAL_MODE_ALTERNATE(2));
	palSetPadMode(gpio, p22, PAL_MODE_ALTERNATE(2));
	
	chBSemInit(&motor_left_sem, FALSE);
	chBSemInit(&motor_right_sem, FALSE);
	
	//disable motors
	pwmStart(motor_pwm, &motor_pwmcfg);
	pwmEnableChannel(motor_pwm, lp1, PWM_PERCENTAGE_TO_WIDTH(motor_pwm,0));
	pwmEnableChannel(motor_pwm, lp2, PWM_PERCENTAGE_TO_WIDTH(motor_pwm,0));
	pwmEnableChannel(motor_pwm, rp1, PWM_PERCENTAGE_TO_WIDTH(motor_pwm,0));
	pwmEnableChannel(motor_pwm, rp2, PWM_PERCENTAGE_TO_WIDTH(motor_pwm,0));
	return TRUE;
}

//set motor_speed
int motorSetSpeed(int s){
	chSysLock();
	if(25 <= s && s <= 100){
		motor_speed = 100*s;
		chSysUnlock();
		return TRUE;
	}
	chSysUnlock();
	return FALSE;

}

//enable motors for specific time
void motorLeftForwardS(int milli){
	motor_left_on(D_FORWARD);
	chVTSet(&vtl, MS2ST(milli), motor_left_off, NULL);
	
}
void motorLeftBackwardS(int milli){
	motor_left_on(D_BACKWARD);
	chVTSet(&vtl, MS2ST(milli), motor_left_off, NULL);
}

void motorRightForwardS(int milli){
	motor_right_on(D_FORWARD);
	chVTSet(&vtr, MS2ST(milli), motor_right_off, NULL);
	
}
void motorRightBackwardS(int milli){
	motor_right_on(D_BACKWARD);
	chVTSet(&vtr, MS2ST(milli), motor_right_off, NULL);
}

void motorForwardS(int milli){
	motorLeftForwardS(milli);
	motorRightForwardS(milli);
}
void motorBackwardS(int milli){
	motorLeftBackwardS(milli);
	motorRightBackwardS(milli);
}

//disable motors
void motorLeftOff(void){
	chSysLock();
	pwmEnableChannelI(motor_pwm, lp1, PWM_PERCENTAGE_TO_WIDTH(motor_pwm,0));
	pwmEnableChannelI(motor_pwm, lp2, PWM_PERCENTAGE_TO_WIDTH(motor_pwm,0));
	chBSemSignalI(&motor_left_sem);
	chSysUnlock();
}

void motorRightOff(void){
	chSysLock();
	pwmEnableChannelI(motor_pwm, rp1, PWM_PERCENTAGE_TO_WIDTH(motor_pwm,0));
	pwmEnableChannelI(motor_pwm, rp2, PWM_PERCENTAGE_TO_WIDTH(motor_pwm,0));
	chBSemSignalI(&motor_right_sem);
	chSysUnlock();
}

void motorOff(void){
	motorLeftOff();
	motorRightOff();
}

//check if motors are active
int motorLeftActive(void){
	int ret;
	chSysLock();
	ret = chBSemGetStateI(&motor_left_sem);
	chSysUnlock();
	return ret;
	
}
int motorRightActive(void){
	int ret;
	chSysLock();
	ret = chBSemGetStateI(&motor_right_sem);
	chSysUnlock();
	return ret;
}
int motorActive(void){
	return (motorLeftActive() && motorRightActive());
}

/* PRIVATE METHODS */
//enable motors
void motor_left_on(Direction dir){
	chBSemWait(&motor_left_sem);
	chSysLock();
	if(dir == D_FORWARD){
		pwmEnableChannelI(motor_pwm, lp1, PWM_PERCENTAGE_TO_WIDTH(motor_pwm,0));
		pwmEnableChannelI(motor_pwm, lp2, PWM_PERCENTAGE_TO_WIDTH(motor_pwm,motor_speed));
	}else{
		pwmEnableChannelI(motor_pwm, lp1, PWM_PERCENTAGE_TO_WIDTH(motor_pwm,motor_speed));
		pwmEnableChannelI(motor_pwm, lp2, PWM_PERCENTAGE_TO_WIDTH(motor_pwm,0));
	}
	chSysUnlock();
}

void motor_right_on(Direction dir){
	chBSemWait(&motor_right_sem);
	chSysLock();
	if(dir == D_FORWARD){
		pwmEnableChannelI(motor_pwm, rp1, PWM_PERCENTAGE_TO_WIDTH(motor_pwm,0));
		pwmEnableChannelI(motor_pwm, rp2, PWM_PERCENTAGE_TO_WIDTH(motor_pwm,motor_speed));
	}else{
		pwmEnableChannelI(motor_pwm, rp1, PWM_PERCENTAGE_TO_WIDTH(motor_pwm,motor_speed));
		pwmEnableChannelI(motor_pwm, rp2, PWM_PERCENTAGE_TO_WIDTH(motor_pwm,0));
	}
	chSysUnlock();
}

//disable motors (interrupt)
void motor_left_off(void *p) {
	(void) p;
	chSysLockFromIsr();
	pwmEnableChannelI(motor_pwm, lp1, PWM_PERCENTAGE_TO_WIDTH(motor_pwm,0));
	pwmEnableChannelI(motor_pwm, lp2, PWM_PERCENTAGE_TO_WIDTH(motor_pwm,0));
	chBSemSignalI(&motor_left_sem);
	chSysUnlockFromIsr();
}

void motor_right_off(void *p) {
	(void) p;
	chSysLockFromIsr();
	pwmEnableChannelI(motor_pwm, rp1, PWM_PERCENTAGE_TO_WIDTH(motor_pwm,0));
	pwmEnableChannelI(motor_pwm, rp2, PWM_PERCENTAGE_TO_WIDTH(motor_pwm,0));
	chBSemSignalI(&motor_right_sem);
	chSysUnlockFromIsr();
}

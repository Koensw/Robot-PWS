/* 
 * PWS-Robot
 * (c) 2012 Koen Wolters en Erik Kooistra
 * 
 * Regelt de communicatie met de WiFly
 * 
 * Gebruikt driver D2
 */

#ifndef _SERV_WIFLY_H
#define _SERV_WIFLY_H

#include "ch.h"
#include "hal.h"

typedef enum {S_OK, S_ERROR, S_OVERFLOW, S_TIMEOUT, S_RESET} State;
typedef enum {M_OK, M_ERROR, M_MESSAGE} Message;

typedef Message (*ProtocolCallback)(char id[10], char args[1024]);

/* PUBLIC METHODS */
/*
 * Initialize
 * serial = driver to use
 * ioportid_t = port to use rx and tx port
 * rx = rx port
 * tx = tx port
 * baud = baudrate
 * func = function to callback when data is received (and no listener)
 */
int wiflyInit(SerialDriver *serial, ioportid_t gpio, int rx, int tx, int baud, ProtocolCallback func);
/*
 * Send message
 */ 
State wiflySend(char *msg);
/*
 * Read answer command (WARNING: lines should be at most 128 bytes and all should fit in total)
 */
State wiflyReadCommand(char *cmd, char *total);
/*
 * Send command and check if answer is expected (WARNING: expect answer to be at most 128 bytes)
 */
State wiflySendCommandOK(char *cmd);
State wiflySendCommand(char *cmd, const char *ans);
/*
 * Read message (WARNING: 128 bytes are read maximal)
 */
State wiflyRead(char *cmd);
/*
 * Connect to network
 */
State wiflyConnect(char *ssid, char *phrase);

/* PRIVATE METHODS */
//send
void wifly_send(char *msg);
//receive
State wifly_receive(char *cmd, int MAX_IN);
//wifly thread loop
msg_t wifly_loop(void *arg);

#endif

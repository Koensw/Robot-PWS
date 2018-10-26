#include "serv_wifly.h"
#include "string.h"

#define WIFLY_MEM_SIZE 10
#define WIFLY_EL_SIZE 1024

SerialDriver *wifly_driver;
ProtocolCallback wifly_rec_func;

// wifly_config
SerialConfig wifly_config = {9600, 0, USART_CR2_STOP1_BITS | USART_CR2_LINEN,
                             0};

EventListener wifly_el;

MemoryPool wifly_pool;
char char_mem[WIFLY_MEM_SIZE][WIFLY_EL_SIZE]
    __attribute__((aligned(sizeof(stkalign_t))));
;

Thread *wifly_tp;
static WORKING_AREA(wifly_thread_area, 4096);

// initalizes wifly
int wiflyInit(SerialDriver *serial, ioportid_t gpio, int tx, int rx, int baud,
              ProtocolCallback func) {
  wifly_driver = serial;
  wifly_config.sc_speed = baud;
  wifly_rec_func = func;

  sdStart(wifly_driver, &wifly_config);
  palSetPadMode(GPIOD, GPIOD_LED6,
                PAL_MODE_OUTPUT_PUSHPULL); // (FIXME: DEBUGGING)
  palSetPadMode(gpio, tx, PAL_MODE_ALTERNATE(7));
  palSetPadMode(gpio, rx, PAL_MODE_ALTERNATE(7));

  // initialize memory pool
  chPoolInit(&wifly_pool, WIFLY_EL_SIZE, NULL);
  chPoolLoadArray(&wifly_pool, char_mem, WIFLY_MEM_SIZE);

  // start listening thread
  wifly_tp = chThdCreateStatic(wifly_thread_area, sizeof(wifly_thread_area),
                               NORMALPRIO - 1, (tfunc_t)wifly_loop, NULL);

  wiflySend("****INIT***\r\n");
  // FIXME: test if working
  return TRUE;
}

// send message
State wiflySend(char *msg) {
  if (strlen(msg) == 0)
    return S_OVERFLOW; // not correct length
  // ask thread to send message (if current thread is not executing this)
  if (chThdSelf() == wifly_tp)
    wifly_send(msg);
  else
    chMsgSend(wifly_tp, (msg_t)msg);
  return S_OK;
}

// read answer of command
// TODO: fix extra '\n'
State wiflyReadCommand(char *cmd, char *total) {
  char *buf = chPoolAlloc(&wifly_pool);
  buf[0] = '\0';
  total[0] = '\0';

  // FIXME: DEBUG
  wiflySend("***COMMAND - ");
  wiflySend(cmd);
  wiflySend(" - ***\r\n");
  chThdSleepMilliseconds(100);
  wiflySend("$$$");

  wiflyRead(buf);
  wiflyRead(buf);
  /*if(strcmp(buf, "CMD") != 0){
          wiflySend("\r\nexit\r\n");
          chPoolFree(&wifly_pool, buf);
          return S_ERROR;
  }*/
  wiflySend(cmd);
  wiflySend("\r\n");
  wiflyRead(buf); // skip return line
  while (1) {
    State s = wiflyRead(buf);         // read actual info
    if (s == S_OK && buf[0] != '<') { // skip empty lines and commands
      strcat(total, buf);
      strcat(total, "\r\n");
    } else
      break;
  }

  wiflySend("exit\r\n");
  wiflyRead(buf); // skip further (TODO: skip iqueue ??)
  wiflyRead(buf); // skip further
  /*if(strcmp(buf, "EXIT") != 0){
          chPoolFree(&wifly_pool, buf);
          return S_ERROR;
  }*/
  // skip last newline
  total[strlen(total) - 2] = '\0';
  chPoolFree(&wifly_pool, buf);
  return S_OK;
}

// send command
State wiflySendCommandOK(char *cmd) { return wiflySendCommand(cmd, "AOK"); }

State wiflySendCommand(char *cmd, const char *ans) {
  char *buf = chPoolAlloc(&wifly_pool);
  buf[0] = '\0';
  State s = wiflyReadCommand(cmd, buf);
  if (strcmp(ans, buf) == 0) {
    chPoolFree(&wifly_pool, buf);
    return s;
  }
  chPoolFree(&wifly_pool, buf);
  return S_ERROR;
}

// read message into static buffer
State wiflyRead(char *msg) {
  msg[0] = 0;
  // ask thread to receive message (if current thread is not executing this)
  if (chThdSelf() == wifly_tp)
    return wifly_receive(msg, WIFLY_EL_SIZE);
  else
    return chMsgSend(wifly_tp, (msg_t)msg);
}

// connect to wifly
State wiflyConnect(char *ssid, char *phrase) {
  State s = S_OK;
  s = wiflySendCommand("set wlan auth 3", "AOK");
  if (s != S_OK)
    return S_ERROR;
  s = wiflySendCommand("set ip dhcp 1", "AOK");
  if (s != S_OK)
    return S_ERROR;
  s = wiflySendCommand("set wlan join 1", "AOK");
  if (s != S_OK)
    return S_ERROR;
  s = wiflySendCommand("set wlan channel 0", "AOK");
  if (s != S_OK)
    return S_ERROR;

  char *buf = chPoolAlloc(&wifly_pool);
  buf[0] = '\0';
  strcpy(buf, "set wlan phrase ");
  strcat(buf, phrase);
  s = wiflySendCommand(buf, "AOK");
  if (s != S_OK) {
    chPoolFree(&wifly_pool, buf);
    return S_ERROR;
  }
  strcpy(buf, "set wlan ssid ");
  strcat(buf, ssid);
  s = wiflySendCommand(buf, "AOK");
  if (s != S_OK) {
    chPoolFree(&wifly_pool, buf);
    return S_ERROR;
  }

  s = wiflySendCommand("save", "Storing in config");
  if (s != S_OK) {
    chPoolFree(&wifly_pool, buf);
    return S_ERROR;
  }
  s = wiflySendCommand("reboot", "AOK");
  if (s != S_OK) {
    chPoolFree(&wifly_pool, buf);
    return S_ERROR;
  }
  chPoolFree(&wifly_pool, buf);
  return S_OK;
}

void wifly_send(char *msg) {
  if (strlen(msg) == 0)
    return;
  sdWrite(wifly_driver, (uint8_t *)msg, strlen(msg));
  // wait for empty buffer
  while (TRUE) {
    chEvtWaitOne(1);

    flagsmask_t flags;
    flags = chEvtGetAndClearFlags(&wifly_el);
    if (flags & CHN_OUTPUT_EMPTY)
      break;
  }
}

State wifly_receive(char *msg, int max_l) {
  State s = 0;

  int i = 0;
  int finished = FALSE;
  // receive all that is already on the queue
  while (TRUE) {
    int in = sdGetTimeout(wifly_driver, TIME_IMMEDIATE);
    if (in == Q_TIMEOUT || in == Q_RESET)
      break;

    if (in != '\r' && in != '\n') {
      msg[i] = in;
      ++i;
    }

    if (i == max_l - 1) {
      s = S_OVERFLOW;
      finished = TRUE;
      chIQResetI(&(wifly_driver)->iqueue);
      break;
    } else if (in == '\n') {
      finished = TRUE;
      break;
    }
  }
  // read all other chars
  while (!finished) {
    if (chEvtWaitOneTimeout(1, MS2ST(1000)) == 0) {
      s = S_TIMEOUT;
      break;
    };

    flagsmask_t flags;
    flags = chEvtGetAndClearFlags(&wifly_el);
    if (flags & CHN_INPUT_AVAILABLE) {
      int in = sdGetTimeout(wifly_driver, TIME_IMMEDIATE);
      if (in == Q_TIMEOUT || in == Q_RESET) {
        s = S_RESET;
        break;
      }

      if (in != '\r' && in != '\n') {
        msg[i] = in;
        ++i;
      }

      if (i == max_l - 1) {
        s = S_OVERFLOW;
        chIQResetI(&(wifly_driver)->iqueue);
        break;
      } else if (in == '\n')
        break;

    } else if (flags > 32) {
      s = S_ERROR;
      break;
    }
  }
  msg[i] = '\0';
  return s;
}

msg_t wifly_loop(void *arg) {
  (void)arg;

  chEvtRegisterMask((EventSource *)chnGetEventSource(wifly_driver), &wifly_el,
                    1);

  int i = 0;
  int j = 0;
  int read_id = TRUE;
  char id[16] = "";
  id[0] = id[1] = 0;
  char args[1024] = "";
  args[0] = args[1] = 0;

  while (TRUE) {
    // give time to other threads
    chThdYield();

    // check if incoming messages for thread (if not reading message)
    msg_t msg;
    if (i == 0 && j == 0 && chMsgIsPendingI(wifly_tp)) {
      // receive message
      Thread *msg_thd = chMsgWait();
      msg = chMsgGet(msg_thd);
      // check if message to send or message to receive
      char *str = (char *)msg;
      if (strlen(str) == 0) {
        // receive message
        State st = wifly_receive(str, WIFLY_EL_SIZE);
        chMsgRelease(msg_thd, (msg_t)st);
      } else {
        // send message
        wifly_send(str);
        chMsgRelease(msg_thd, (msg_t)NULL);
      }
    }

    // wait for event
    if (chEvtWaitOneTimeout(1, TIME_IMMEDIATE) == 0)
      continue;

    flagsmask_t flags;
    flags = chEvtGetAndClearFlags(&wifly_el);
    if (flags & CHN_INPUT_AVAILABLE) {
      int in = sdGetTimeout(wifly_driver, TIME_IMMEDIATE);

      if (read_id) {
        if (in == ' ')
          read_id = FALSE;
        else if (in != '\r' && in != '\n') {
          id[i] = in;
          ++i;
        }
      } else {
        if (in != '\r' && in != '\n') {
          args[j] = in;
          ++j;
        }
      }

      if (i == 16 || j == 1024) {
        // clear buffer
        wifly_send("*OVERFLOW*\r\n");
        chIQResetI(&(wifly_driver)->iqueue);
        i = 0;
        j = 0;
        read_id = TRUE;
      } else if (in == '\n') {
        id[i] = args[j] = 0;
        if (i > 2 && id[0] == '*' && id[i - 1] == '*') {
          // state
          if (strcmp(id, "*HELLO*") == 0)
            wifly_send("*HELLO*\r\n");
          else
            wifly_send("*ERROR*\r\n");
        } else {
          // message
          Message ret = wifly_rec_func(id, args);
          if (ret == M_OK)
            wifly_send("*OK*\r\n");
          else if (ret != M_MESSAGE)
            wifly_send("*ERROR*\r\n");
        }

        // read next
        i = 0;
        j = 0;
        read_id = TRUE;
      }
    } else if (flags > 32) {
      // set error led (FIXME: DEBUGGING)
      palSetPad(GPIOD, GPIOD_LED6);
      // reset queues
      chIQResetI(&(wifly_driver)->iqueue);
      chOQResetI(&(wifly_driver)->oqueue);
    }
  }
  return 0;
}

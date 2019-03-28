
/*
 * Copyright Bitmain Technologies Inc.
 * Written by:
 *   XianFeng Kuang<xianfeng.kuang@bitmain.com>
 * Created Time: 2018-08-10 11:34
 */

#ifndef BM_USBTTY_H

#define BM_USBTTY_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <limits.h>
#include <sys/file.h>
#include <errno.h>
#include <stdint.h>
#include <inttypes.h>
#include <syslog.h>
#include "ipc_configs.h"

int tty_open_device(char *devName, int nBuadRate);
int tty_flush(void);
int tty_send(unsigned char *pBuf, unsigned char *pCmd, unsigned int nLen, FormatType type);
int tty_recv_header(bm_usb_header_tx_pkt *pRxPkt, unsigned int timeout_ms);
int tty_recv_buffer(unsigned char *pBuf, unsigned int nLen, unsigned int timeout_ms);
int tty_is_opened(void);
void tty_close_device(void);
int tty_is_enabled(void);
void tty_enable(void);
void tty_disable(void);

#endif // BM_USBTTY_H

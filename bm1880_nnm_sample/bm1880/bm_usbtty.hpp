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

class UsbTTyDevice
{
	public:
		UsbTTyDevice(char *devName, int nBuadRate);
		int OpenDevice(char *devName, int nBuadRate);
		int IsOpened() { return devFd > 0 ? 1 : 0; };

		~UsbTTyDevice()
		{
			if (devFd > 0)
				close(devFd);
		};

		int Flush();
		int SendBuf(unsigned char *pBuf, unsigned char *pCmd, unsigned int nLen, FormatType type, IpcRetCode code);
		int RecvBuf(unsigned char *pBuf, bm_usb_header_tx_pkt *pRxHdr);

		int ValidateRecvBufPkt(unsigned char *pBuf, unsigned int nLen, unsigned int timeout_ms);
		int ValidateRecvBufPkt64k(unsigned char *pBuf, unsigned int nLen, unsigned int timeout_ms);
		int ValidateRecvBufPktHeader(bm_usb_header_tx_pkt *pRxPkt, unsigned int timeout_ms);

		int SendBufPktHeader(unsigned char *pBuf, unsigned int nLen, FormatType type, IpcRetCode code);
		int SendBufPkt(unsigned char *pBuf, unsigned int nLen);
		int SendBufPkt64k(unsigned char *pBuf, unsigned int nLen);

	private:
		int WriteBuf(unsigned char *pBuf, unsigned int nLen);
		int RecvBufPkt(unsigned char *pBuf, unsigned int nLen, unsigned int timeout_ms);
		int ReadBuf(unsigned char *pBuf, unsigned int nLen);

		int PollReadTimeout(unsigned char *pBuf, unsigned int nLen, unsigned int timeout_ms);

		int SendAckPkt(void);
		int ValidateRecvAckPkt(unsigned int timeout_ms);

		int devFd;
		struct termios new_settings, old_settings;
};

#endif // BM_USBTTY_H

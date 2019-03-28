/*
 * Copyright Bitmain Technologies Inc.
 * Written by:
 *   XianFeng Kuang<xianfeng.kuang@bitmain.com>
 * Created Time: 2018-08-10 11:34
 */

#include "bm_usbtty.hpp"

#define TIMEOUT_ERR -2

int UsbTTyDevice::OpenDevice(char *devName, int nBuadRate)
{

	devFd = open(devName, O_RDWR | O_NOCTTY);
	if (devFd < 0)
	{
		perror("unable to open comport ");
		return (-1);
	}

	int baudr;
	switch (nBuadRate)
	{
		case 50:
			baudr = B50;
			break;
		case 75:
			baudr = B75;
			break;
		case 110:
			baudr = B110;
			break;
		case 134:
			baudr = B134;
			break;
		case 150:
			baudr = B150;
			break;
		case 200:
			baudr = B200;
			break;
		case 300:
			baudr = B300;
			break;
		case 600:
			baudr = B600;
			break;
		case 1200:
			baudr = B1200;
			break;
		case 1800:
			baudr = B1800;
			break;
		case 2400:
			baudr = B2400;
			break;
		case 4800:
			baudr = B4800;
			break;
		case 9600:
			baudr = B9600;
			break;
		case 19200:
			baudr = B19200;
			break;
		case 38400:
			baudr = B38400;
			break;
		case 57600:
			baudr = B57600;
			break;
		case 115200:
			baudr = B115200;
			break;
		case 230400:
			baudr = B230400;
			break;
		case 460800:
			baudr = B460800;
			break;
		case 500000:
			baudr = B500000;
			break;
		case 576000:
			baudr = B576000;
			break;
		case 921600:
			baudr = B921600;
			break;
		case 1000000:
			baudr = B1000000;
			break;
		case 1152000:
			baudr = B1152000;
			break;
		case 1500000:
			baudr = B1500000;
			break;
		case 2000000:
			baudr = B2000000;
			break;
		case 2500000:
			baudr = B2500000;
			break;
		case 3000000:
			baudr = B3000000;
			break;
		case 3500000:
			baudr = B3500000;
			break;
		case 4000000:
			baudr = B4000000;
			break;
		default:
			DEBUG_LOG("invalid baudrate\n");
			return (1);
			break;
	}

	if (flock(devFd, LOCK_EX | LOCK_NB) != 0)
	{
		close(devFd);
		perror("Another process has locked the comport.");
		return (-1);
	}

	int error = tcgetattr(devFd, &old_settings);

	if (error == -1)
	{
		close(devFd);
		flock(devFd, LOCK_UN); /* free the port so that others can use it. */
		perror("unable to read portsettings ");
		return (-1);
	}

	memset(&new_settings, 0, sizeof(new_settings)); /* clear the new struct */

	new_settings.c_cflag |= (CLOCAL | CREAD);
	new_settings.c_lflag |= ~(ICANON | ECHO | ECHOE | ISIG);
	new_settings.c_oflag |= ~OPOST;
	new_settings.c_cc[VMIN] = 0;
	new_settings.c_cc[VTIME] = 0;

	cfsetispeed(&new_settings, baudr);
	cfsetospeed(&new_settings, baudr);

	error = tcsetattr(devFd, TCSANOW, &new_settings);

	if (error == -1)
	{
		tcsetattr(devFd, TCSANOW, &old_settings);
		close(devFd);
		flock(devFd, LOCK_UN); /* free the port so that others can use it. */
		perror("unable to adjust portsettings ");
		return (-1);
	}

	return 0;
}

UsbTTyDevice::UsbTTyDevice(char *devName, int nBuadRate)
{
	OpenDevice(devName, nBuadRate);
}

int UsbTTyDevice::ReadBuf(unsigned char *pBuf, unsigned int nLen)
{
	int n;

	n = read(devFd, pBuf, nLen);

	if (n < 0)
	{
		if (errno == EAGAIN)
		{
			return -1;
		}
	}

	return (n);
}

int UsbTTyDevice::Flush()
{
	return tcflush(devFd, TCIOFLUSH);
}

int UsbTTyDevice::WriteBuf(unsigned char *pBuf, unsigned int nLen)
{
	//make buffer clean
	//tcflush(devFd, TCOFLUSH);
	int n = write(devFd, pBuf, nLen);

	if (n < 0)
	{
		if (errno == EAGAIN)
		{
			return 0;
		}
		else
		{
			return -1;
		}
	}

	return (n);
}

int UsbTTyDevice::SendBuf(unsigned char *pBuf, unsigned char *pCmd, unsigned int nLen, FormatType type, IpcRetCode code)
{
	int ret = SendBufPktHeader(pCmd, nLen, type, code);

	if (ret)
	{
		return -1;
	}
	int txbytes = SendBufPkt(pBuf, nLen);

	return txbytes;
}

int UsbTTyDevice::RecvBuf(unsigned char *pBuf, bm_usb_header_tx_pkt *pRxHdr)
{
	int ret = ValidateRecvBufPktHeader(pRxHdr, 3);

	if (ret < 0)
	{
		return -1;
	}
	//wait and recv buffer

	ret = ValidateRecvBufPkt(pBuf, pRxHdr->data_len, 3) < 0;

	if (ret < 0)
	{
		return -1;
	}

	return pRxHdr->data_len;
}

int UsbTTyDevice::SendBufPkt(unsigned char *pBuf, unsigned int nLen)
{
	uint32_t tx_bytes = 0;

	// Send buffer
	tx_bytes = WriteBuf(pBuf, nLen);

	if (tx_bytes != nLen)
	{
		fprintf(stderr, "tx header fail %u\n", tx_bytes);
		return -1;
	}

	return tx_bytes;
}

int UsbTTyDevice::SendBufPkt64k(unsigned char *pBuf, unsigned int nLen)
{
	uint32_t tx_bytes = 0;

	const unsigned int cut_size = 60 * 1024;
	unsigned int remain_size = nLen;
	unsigned int send_size = 0;
	unsigned char *pdata = pBuf;

	while (remain_size > 0)
	{
		send_size = (remain_size < cut_size) ? remain_size : cut_size;
		tx_bytes = WriteBuf(pdata, send_size);

		if (tx_bytes != send_size)
		{
			fprintf(stderr, "tx header fail %u\n", tx_bytes);
			return -1;
		}
		remain_size -= send_size;
		pdata += send_size;

		// Check ACK
		while (0 != ValidateRecvAckPkt(3))
		{
			;
		}
	}

	return nLen;
}

int UsbTTyDevice::SendBufPktHeader(unsigned char *pCmdBuf, unsigned int nRawDataLen, FormatType type, IpcRetCode code)
{
	bm_usb_header_tx_pkt h_tx_pkt;
	h_tx_pkt.magic[0] = 'B';
	h_tx_pkt.magic[1] = 'M';
	h_tx_pkt.magic[2] = 'T';
	h_tx_pkt.magic[3] = 'X';
	h_tx_pkt.data_len = nRawDataLen;
	h_tx_pkt.type = type;
	h_tx_pkt.ret_code = code;
	memset(h_tx_pkt.cmd, 0, MAX_CMD_LEN);
	memcpy(h_tx_pkt.cmd, pCmdBuf, strlen((const char *)pCmdBuf));

	SendBufPkt((unsigned char *)&h_tx_pkt, sizeof(bm_usb_header_tx_pkt));
	return 0;
}

int UsbTTyDevice::RecvBufPkt(unsigned char *pBuf, unsigned int nLen, unsigned int timeout_ms)
{
	int recv_bytes = 0;
	int rx_bytes = 0;

	while (recv_bytes != (int)nLen)
	{
		rx_bytes = PollReadTimeout(pBuf + recv_bytes, nLen - recv_bytes, timeout_ms);
		if (rx_bytes > 0)
		{
			recv_bytes += rx_bytes;
		}
	}

	return recv_bytes;
}

int UsbTTyDevice::SendAckPkt(void)
{
	bm_usb_ack_pkt ack_pkt;

	ack_pkt.magic[0] = 'B';
	ack_pkt.magic[1] = 'A';
	ack_pkt.magic[2] = 'C';
	ack_pkt.magic[3] = 'K';

	// Send Header
	int tx_bytes = WriteBuf((unsigned char *)&ack_pkt, sizeof(bm_usb_ack_pkt));

	if (tx_bytes != sizeof(bm_usb_ack_pkt))
	{
		fprintf(stderr, "ack send fail %u\n", tx_bytes);
		return -1;
	}
	return 0;
}

int UsbTTyDevice::ValidateRecvAckPkt(unsigned int timeout_ms)
{
	bm_usb_ack_pkt ack_pkt;
	memset(&ack_pkt, 0, sizeof(bm_usb_ack_pkt));
	int size = RecvBufPkt((unsigned char *)&ack_pkt, sizeof(bm_usb_ack_pkt), timeout_ms);
	if (size == sizeof(bm_usb_ack_pkt) && (ack_pkt.magic[0] == 'B' && ack_pkt.magic[1] == 'A' && ack_pkt.magic[2] == 'C' && ack_pkt.magic[3] == 'K'))
	{
		return 0;
	}
	return -1;
}

int UsbTTyDevice::ValidateRecvBufPkt(unsigned char *pBuf, unsigned int nLen, unsigned int timeout_ms)
{
	int size = RecvBufPkt((unsigned char *)pBuf, nLen, timeout_ms);
	if (size == (int)nLen)
	{
		return 0;
	}
	else
	{
		return -1;
	}
}

int UsbTTyDevice::ValidateRecvBufPkt64k(unsigned char *pBuf, unsigned int nLen, unsigned int timeout_ms)
{
	const unsigned int cut_size = 60 * 1024;
	unsigned int remain_size = nLen;
	unsigned int receive_size = 0;
	unsigned char *pdata = pBuf;
	while (remain_size > 0)
	{
		receive_size = (remain_size < cut_size) ? remain_size : cut_size;
		RecvBufPkt((unsigned char *)pdata, receive_size, 3);
		remain_size -= receive_size;
		pdata += receive_size;
		SendAckPkt();
	}
	return 0;
}

int UsbTTyDevice::ValidateRecvBufPktHeader(bm_usb_header_tx_pkt *pRxPkt, unsigned int timeout_ms)
{
	int size = RecvBufPkt((unsigned char *)pRxPkt, sizeof(bm_usb_header_tx_pkt), timeout_ms);
	if (size == sizeof(bm_usb_header_tx_pkt) && (pRxPkt->magic[0] == 'B' && pRxPkt->magic[1] == 'M' && pRxPkt->magic[2] == 'T' && pRxPkt->magic[3] == 'X'))
	{
		return 0;
	}
	else
	{
		return -1;
	}
}

int UsbTTyDevice::PollReadTimeout(unsigned char *pBuf, unsigned int nLen, unsigned int timeout_ms)
{
	fd_set rfds;
	struct timeval tv;
	int retval;
	int n = 0;

	/* Watch stdin (fd 0) to see when it has input. */

	FD_ZERO(&rfds);
	FD_SET(devFd, &rfds);

	/* Wait up to five seconds. */

	tv.tv_sec = 0;
	tv.tv_usec = timeout_ms * 1000;

	retval = select(devFd + 1, &rfds, NULL, NULL, &tv);
	/* Don't rely on the value of tv now! */

	if (retval == -1)
	{
		perror("select()");
	}
	else if (retval == 0)
	{
		n = TIMEOUT_ERR;
	}
	else
	{
		n = ReadBuf(pBuf, nLen);
	}

	return n;
}

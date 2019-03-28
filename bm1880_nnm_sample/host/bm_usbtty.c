#include "bm_usbtty.h"
#include "sys/select.h"
#include <pthread.h>

static int devFd = -1;
static int enabled = 1;
//static pthread_t tid;

static int heart_beats = 1;
static int old_heart_beats = 0;

static struct termios new_settings, old_settings;

int tty_send_buffer_pkt_header(unsigned char *pBuf, unsigned int nLen, FormatType type);
int tty_send_buffer_pkt(unsigned char *pBuf, unsigned int nLen);
int tty_send_buffer_pkt_64k(unsigned char *pBuf, unsigned int nLen);

int tty_read_buffer(unsigned char *pBuf, unsigned int nLen);
int tty_write_buffer(unsigned char *pBuf, unsigned int nLen);
int tty_recv_buffer_pkt(unsigned char *pBuf, unsigned int nLen, unsigned int timeout_ms);
int tty_recv_buffer_pkt_64k(unsigned char *pBuf, unsigned int nLen, unsigned int timeout_ms);
int tty_poll_and_read(unsigned char *pBuf, unsigned int nLen, unsigned int timeout_ms);

// utilities to send / receive tty ack
int tty_send_ack(void);
int tty_recv_ack(unsigned int timeout_ms);

void snapshot_beats(void)
{
	old_heart_beats = heart_beats;
}

void beat(void)
{
	++heart_beats;
}

void *check_freeze(void *arg)
{
	while (devFd != -1)
	{
		sleep(3);
		printf("check_freeze %d %d\n", old_heart_beats, heart_beats);
		if (old_heart_beats == heart_beats)
		{
			printf("\n\n\n------------------------- freeze. start reset --------------------------\n\n\n");
			tty_disable();
			sleep(3);
			tty_flush();
			tty_enable();
			++heart_beats;
		}
	}
	return NULL;
}

int tty_open_device(char *devName, int nBuadRate)
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
			printf("invalid baudrate\n");
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

	new_settings.c_iflag |= (IXON | IXOFF);

	cfsetispeed(&new_settings, baudr);
	cfsetospeed(&new_settings, baudr);

	error = tcsetattr(devFd, TCSANOW, &new_settings);

	if (error == -1)
	{
		tcsetattr(devFd, TCSANOW, &old_settings);
		close(devFd);
		flock(devFd, LOCK_UN); /* free the port so that others can use it. */
		perror("unable to adjust portsettings ");
		return -1;
	}
	//pthread_create(&tid, NULL, check_freeze, NULL);
	return 0;
}

int tty_flush()
{
	if (devFd < 0)
	{
		return -1;
	}
	return tcflush(devFd, TCIOFLUSH);
}

int tty_send(unsigned char *pBuf, unsigned char *pCmd, unsigned int nLen, FormatType type)
{
	if (devFd < 0)
	{
		return -1;
	}

	snapshot_beats();
	int ret = tty_send_buffer_pkt_header(pCmd, nLen, type);
	beat();

	if (ret == 0)
	{
		snapshot_beats();
		ret = tty_send_buffer_pkt(pBuf, nLen);
		beat();
	}
	return ret;
}

int tty_recv_header(bm_usb_header_tx_pkt *pRxPkt, unsigned int timeout_ms)
{
	if (devFd < 0)
	{
		return -1;
	}

	snapshot_beats();
	int size = tty_recv_buffer_pkt((unsigned char *)pRxPkt, sizeof(bm_usb_header_tx_pkt), timeout_ms);
	beat();

	if (size == sizeof(bm_usb_header_tx_pkt) && (pRxPkt->magic[0] == 'B' && pRxPkt->magic[1] == 'M' && pRxPkt->magic[2] == 'T' && pRxPkt->magic[3] == 'X'))
	{
		return 0;
	}
	else
	{
		return -1;
	}
}

int tty_recv_buffer(unsigned char *pBuf, unsigned int nLen, unsigned int timeout_ms)
{
	if (devFd < 0)
	{
		return -1;
	}

	snapshot_beats();
	int size = tty_recv_buffer_pkt((unsigned char *)pBuf, nLen, timeout_ms);
	beat();

	if (size == (int)nLen)
	{
		return 0;
	}
	else
	{
		return -1;
	}
}

int tty_is_enabled(void)
{
	return enabled;
}

void tty_enable(void)
{
	enabled = 1;
}

void tty_disable(void)
{
	enabled = 0;
}

// internal api
int tty_read_buffer(unsigned char *pBuf, unsigned int nLen)
{
	int n = read(devFd, pBuf, nLen);
	if (n < 0)
	{
		if (errno == EAGAIN)
		{
			return -1;
		}
	}

	return (n);
}

int tty_write_buffer(unsigned char *pBuf, unsigned int nLen)
{
	if (devFd < 0)
	{
		return -1;
	}

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

int tty_send_buffer_pkt(unsigned char *pBuf, unsigned int nLen)
{
	uint32_t tx_bytes = 0;

	// Send buffer
	tx_bytes = tty_write_buffer(pBuf, nLen);

	if (tx_bytes != nLen)
	{
		printf("tx header fail %u\n", tx_bytes);
		return -1;
	}

	return tx_bytes;
}

int tty_send_buffer_pkt_64k(unsigned char *pBuf, unsigned int nLen)
{
	uint32_t tx_bytes = 0;
	const unsigned int cut_size = 60 * 1024;
	unsigned int remain_size = nLen;
	unsigned int send_size = 0;
	unsigned char *pdata = pBuf;

	while (remain_size > 0 && enabled)
	{
		send_size = (remain_size < cut_size) ? remain_size : cut_size;
		tx_bytes = tty_write_buffer(pdata, send_size);

		if (tx_bytes != send_size)
		{
			printf("tx header fail %u\n", tx_bytes);
			return -1;
		}
		remain_size -= send_size;
		pdata += send_size;

		// Check ACK
		while (0 != tty_recv_ack(3))
		{
			;
		}
	}
	return nLen;
}

int tty_send_buffer_pkt_header(unsigned char *pCmdBuf, unsigned int nRawDataLen, FormatType type)
{
	bm_usb_header_tx_pkt h_tx_pkt;
	h_tx_pkt.magic[0] = 'B';
	h_tx_pkt.magic[1] = 'M';
	h_tx_pkt.magic[2] = 'T';
	h_tx_pkt.magic[3] = 'X';
	h_tx_pkt.data_len = nRawDataLen;
	h_tx_pkt.type = type;
	memset(h_tx_pkt.cmd, 0, MAX_CMD_LEN);
	memcpy(h_tx_pkt.cmd, pCmdBuf, strlen((const char *)pCmdBuf));

	tty_send_buffer_pkt((unsigned char *)&h_tx_pkt, sizeof(bm_usb_header_tx_pkt));
	return 0;
}

int tty_recv_buffer_pkt(unsigned char *pBuf, unsigned int nLen, unsigned int timeout_ms)
{
	int recv_bytes = 0;
	int rx_bytes = 0;

	while (recv_bytes != (int)nLen && enabled)
	{
		rx_bytes = tty_poll_and_read(pBuf + recv_bytes, nLen - recv_bytes, timeout_ms);
		if (rx_bytes > 0)
		{
			recv_bytes += rx_bytes;
		}
	}

	return recv_bytes;
}

int tty_recv_buffer_pkt_64k(unsigned char *pBuf, unsigned int nLen, unsigned int timeout_ms)
{
	const unsigned int cut_size = 60 * 1024;
	unsigned int remain_size = nLen;
	unsigned int receive_size = 0;
	unsigned char *pdata = pBuf;
	int rx_bytes = 0;

	while (remain_size > 0 && enabled)
	{
		receive_size = (remain_size < cut_size) ? remain_size : cut_size;
		rx_bytes = tty_poll_and_read((unsigned char *)pdata, receive_size, timeout_ms);
		remain_size -= rx_bytes;
		pdata += rx_bytes;
		tty_send_ack();
	}
	return 0;
}

int tty_poll_and_read(unsigned char *pBuf, unsigned int nLen, unsigned int timeout_ms)
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
		n = tty_read_buffer(pBuf, nLen);
	}

	return n;
}

int tty_is_opened()
{
	return devFd > 0 ? 1 : 0;
}

void tty_close_device()
{
	if (devFd > 0)
	{
		close(devFd);
		devFd = -1;
		//pthread_join(tid, NULL);
	}
};

// utilities to send / receive tty ack
int tty_send_ack(void)
{
	bm_usb_ack_pkt ack_pkt;

	ack_pkt.magic[0] = 'B';
	ack_pkt.magic[1] = 'A';
	ack_pkt.magic[2] = 'C';
	ack_pkt.magic[3] = 'K';

	// Send Header
	int tx_bytes = tty_write_buffer((unsigned char *)&ack_pkt, sizeof(bm_usb_ack_pkt));

	if (tx_bytes != sizeof(bm_usb_ack_pkt))
	{
		printf("ack send fail %u\n", tx_bytes);
		return -1;
	}
	return 0;
}

int tty_recv_ack(unsigned int timeout_ms)
{
	bm_usb_ack_pkt ack_pkt;
	memset(&ack_pkt, 0, sizeof(bm_usb_ack_pkt));
	int size = tty_recv_buffer_pkt((unsigned char *)&ack_pkt, sizeof(bm_usb_ack_pkt), timeout_ms);
	if (size == sizeof(bm_usb_ack_pkt) && (ack_pkt.magic[0] == 'B' && ack_pkt.magic[1] == 'A' && ack_pkt.magic[2] == 'C' && ack_pkt.magic[3] == 'K'))
	{
		return 0;
	}
	return -1;
}

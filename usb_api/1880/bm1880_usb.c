#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include<sys/time.h>

#include "bm1880_usb.h"

//#define BM_DEBUG

#define DEVICE_FILENAME_RX "/dev/bmusb_rx"
#define DEVICE_FILENAME_TX "/dev/bmusb_tx"

#define IOC_MAGIC '\x66'

#define IOCTL_VALSET _IOW(IOC_MAGIC, 0, struct ioctl_arg)
#define IOCTL_VALGET _IOR(IOC_MAGIC, 1, struct ioctl_arg)
#define IOCTL_VALGET_NUM _IOR(IOC_MAGIC, 2, int)
#define IOCTL_VALSET_NUM _IOW(IOC_MAGIC, 3, int)

#define IOCTL_VAL_MAXNR 3

#ifdef BM_DEBUG
#define BM_PRINT(fmt, ...) printf(fmt, ##__VA_ARGS__)
#else
#define BM_PRINT(fmt, ...)
#endif

#define RET_ERR_IF(_cond, ret, fmt, ...)                    \
    do                                                 \
    {                                                  \
        if ((_cond))                                   \
        {                                              \
            printf("ERROR [%s:%d]:" fmt "\n",          \
                   __func__, __LINE__, ##__VA_ARGS__); \
            return ret;                  \
        }                                              \
    } while (0)

struct ioctl_arg
{
    unsigned int reg;
    unsigned int val;
};

static int fd_rx = 0;
static int fd_tx = 0;

static int _usb_open(const char *dev, bool isAsyncMode)
{
    struct ioctl_arg cmd;
    memset(&cmd, 0, sizeof(cmd));
    int num = 1;
    long ret;
    int fd = open(dev, O_RDWR | O_NDELAY);
    if (fd < 0)
    {
        printf("failed opening device\n");
        return -1;
    }

    ret = ioctl(fd, IOCTL_VALGET, &cmd);

    if (ret == -1)
    {
        printf("errno %d\n", errno);
        perror("ioctl");
        close(fd);
        return -1;
    }

    cmd.val = 0xCC;
    ret = ioctl(fd, IOCTL_VALSET, &cmd);
    if (ret == -1)
    {
        printf("errno %d\n", errno);
        perror("ioctl");
        close(fd);
        return -1;
    }

    ret = ioctl(fd, IOCTL_VALGET, &cmd);

    if (ret == -1)
    {
        printf("errno %d\n", errno);
        perror("ioctl");
    }

    num = isAsyncMode ? 1 : 0;
    ret = ioctl(fd, IOCTL_VALSET_NUM, num);
    if (ret == -1)
    {
        printf("errno %d\n", errno);
        perror("ioctl");
        close(fd);
        return -1;
    }

    ret = ioctl(fd, IOCTL_VALGET_NUM, &num);

    if (ret == -1)
    {
        printf("errno %d\n", errno);
        perror("ioctl");
        close(fd);
        return -1;
    }

    return fd;
}

int bm1880_usb_open(void)
{
    fd_rx = _usb_open(DEVICE_FILENAME_RX, true);
    fd_tx = _usb_open(DEVICE_FILENAME_TX, true);

    if ((fd_rx == -1) || (fd_tx == -1))
    {
        return -1;
    }

    return 0;
}

int bm1880_usb_close(void)
{
    close(fd_rx);
    close(fd_tx);

    return 0;
}

size_t bm1880_usb_read(void *buf, size_t count, unsigned int timeout_ms)
{
    size_t rx_sz = 0;
    struct timeval start, now;
    unsigned int diff = 0;

    gettimeofday(&start, 0);

    while(1)
    {
        rx_sz = read(fd_rx, buf, count);

        // got data or error
        if (rx_sz != 0)
        {
            return rx_sz;
        }

        // timeout
        if (timeout_ms)
        {
            gettimeofday(&now, 0);
            diff = 1000000 * (now.tv_sec-start.tv_sec)+ now.tv_usec-start.tv_usec;
            if(diff > timeout_ms * 1000)
            {
                break;
            }
        }

        usleep(10);
    }

    return 0;
}

size_t bm1880_usb_write(const void *buf, size_t count, unsigned int timeout_ms)
{
    size_t tx_sz = 0;
    struct timeval start, now;
    unsigned int diff = 0;

    if (count == 0)
    {
        return 0;
    }

    gettimeofday(&start, 0);

    while(1)
    {
        tx_sz = write(fd_tx, buf, count);

        // got data or error
        if (tx_sz != 0)
        {
            return tx_sz;
        }

        // timeout
        if (timeout_ms)
        {
            gettimeofday(&now, 0);
            diff = 1000000 * (now.tv_sec-start.tv_sec)+ now.tv_usec-start.tv_usec;
            if(diff > timeout_ms * 1000)
            {
                break;
            }
        }

        usleep(10);
    }

    return 0;
}

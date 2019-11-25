#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "bm1880_usb.h"

int main(int argc, char **argv)
{
    const int buf_size = 1024*1024;
    unsigned char buf[buf_size];
    size_t size_rx = 0;
    size_t size_tx = 0;

    const int timeout_ms = 10 * 1000;

    memset(buf, 0, buf_size);

    if (-1 == bm1880_usb_open())
    {
        printf("usb open fail\n");
    }

    size_rx = bm1880_usb_read(buf, buf_size, timeout_ms);
    printf("read %d bytes data\n", size_rx);

    size_tx = bm1880_usb_write(buf, size_rx, timeout_ms);
    printf("write %d bytes data\n", size_tx);

    bm1880_usb_close();

    return 0;
}

#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include "libusb_comm.h"

int main(void)
{
    char *text = "hello world!!!!";
    const int BUFSIZE = 100;
    int length;
    char buf[BUFSIZE];
    memset(buf, 0 ,BUFSIZE);

    libusb_comm_open();

    printf("write data...\n");
    libusb_comm_write(text, strlen(text), &length, 10 * 1000);
    sleep (2);
    printf("read data...\n");
    libusb_comm_read(buf, BUFSIZE, &length, 10 * 1000);

    printf("received data (%d Bytes): %s\n", length, buf);

    libusb_comm_close();

    return 0;
}

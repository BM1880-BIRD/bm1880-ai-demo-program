#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include "libusb_comm.h"

int main(int argc, char **argv)
{
    const int BUF_SIZE = 1024 * 1024;
    
    FILE *fp;
    char data[BUF_SIZE];
    int data_size;
    size_t sz;

    char result[BUF_SIZE];

    int transfer_len;
    int total_len;

    
    // read data from file
    fp = fopen(argv[1], "rb");
    if (fp == 0)
    {
        printf("can not open file. err = %d\n", errno);
        return 0;
    }

    fseek(fp, 0, SEEK_END);
    data_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    sz = fread(data, 1, data_size, fp);
    if (sz < data_size)
    {
        printf("read data error. size = %ld\n", sz);
        return 0;
    }

    fclose(fp);

    // send data to bm1880
    libusb_comm_open();

    printf("send image to bm1880, size = %d...\n", data_size);
    libusb_comm_write(data, data_size, &transfer_len, 10 * 1000);
    
    printf("wait result...\n");
    
    // get result from bm1880
    fp = fopen("./result.png", "wb");
    if (fp == 0)
    {
        printf("can not open file. err = %d\n", errno);
        return 0;
    }

    memset(result, 0 ,BUF_SIZE);
    total_len = 0;

    while (1)
    {
        libusb_comm_read(result, BUF_SIZE, &transfer_len, 10 * 1000);
        
        // check finish signal
        if (transfer_len == 11 && strcmp("NoMoreData", result) == 0)
        {
            printf("data finish\n");
            break;
        }
        sz = fwrite(result, 1, transfer_len, fp);
        total_len += transfer_len;
        printf("received %d Bytes\n", transfer_len);
    }

    fclose(fp);
    printf("result saved to result.png (%d Bytes)\n", total_len);

    libusb_comm_close();

    return 0;
}

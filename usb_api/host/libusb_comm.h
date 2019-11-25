#ifndef _LIBUSB_COMM_H_
#define _LIBUSB_COMM_H_

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    BM_USB_RET_OK = 0,
    BM_USB_RET_NOT_OK
} bm_usb_ret_t;

int libusb_comm_open(void);
int libusb_comm_close(void);
int libusb_comm_read(void *data, size_t len, int *translen, unsigned int timeout_ms);
int libusb_comm_write(void *data, size_t len, int *translen, unsigned int timeout_ms);

#endif

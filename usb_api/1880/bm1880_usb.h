#ifndef _BM1880_USB_H_
#define _BM1880_USB_H_

#include <stdlib.h>
#include <stdint.h>

int bm1880_usb_open(void);
int bm1880_usb_close(void);
size_t bm1880_usb_read(void *buf, size_t count, unsigned int timeout_ms);
size_t bm1880_usb_write(const void *buf, size_t count, unsigned int timeout_ms);

#endif

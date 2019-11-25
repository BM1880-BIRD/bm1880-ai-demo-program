#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <time.h>
#include <libusb-1.0/libusb.h>

#include "libusb_comm.h"

//#define BM_DEBUG

#define USB_BM_VID 0x30B1
#define USB_BM_VID_OLD 0x0559
#define USB_BM_PID_ROM 0x1000
#define USB_BM_PID_FIP 0x1001
#define USB_BM_PID_LINUX 0x1003

#ifdef BM_DEBUG
    #define BM_PRINT(fmt, ...) printf(fmt, ##__VA_ARGS__)
#else
    #define BM_PRINT(fmt, ...)
#endif

#define RET_ERR_IF(_cond, fmt, ...) \
  do { \
    if ((_cond)) { \
      printf("ERROR [%s:%d]:" fmt "\n", \
              __func__, __LINE__, ##__VA_ARGS__); \
      return BM_USB_RET_NOT_OK; \
    } \
  }while(0)

libusb_device_handle *usb_handle = NULL;

libusb_device_handle * LIBUSB_CALL libusb_open_device_with_vid_pid_mod(
        libusb_context *ctx, uint16_t vendor_id, uint16_t product_id)
{
        struct libusb_device **devs;
        struct libusb_device *found = NULL;
        struct libusb_device *dev;
        struct libusb_device_handle *dev_handle = NULL;
        size_t i = 0;
        int r;

        if (libusb_get_device_list(ctx, &devs) < 0)
                return NULL;

        while ((dev = devs[i++]) != NULL) {
                struct libusb_device_descriptor desc;
                r = libusb_get_device_descriptor(dev, &desc);
                if (r < 0)
                        goto out;
                if (desc.idVendor == vendor_id && desc.idProduct == product_id) {
                        found = dev;
                        break;
                }
        }

        if (found) {
                BM_PRINT("device detected!!!!!!\n");
                usleep(500 * 1000);
                r = libusb_open(found, &dev_handle);
                if (r < 0)
                        dev_handle = NULL;
        }

out:
        libusb_free_device_list(devs, 1);
        return dev_handle;
}

int open_usb_device(int vid, int pid, uint32_t timeout)
{
  usb_handle = NULL;
  int status = 0;
  struct timeval start, end;

  BM_PRINT("Waiting for BM1880 USB:%#X_%#X...\n", vid, pid);

  gettimeofday(&start, NULL);
  while(usb_handle == NULL)
  {
    if (timeout != 0)
    {
      gettimeofday(&end, NULL);
      if (((1000000 * (end.tv_sec-start.tv_sec)+ end.tv_usec-start.tv_usec) / (double)1000000) >= timeout)
      {
        RET_ERR_IF(usb_handle == NULL, "get device failed");
        break;
      }

   }

    usb_handle = libusb_open_device_with_vid_pid_mod(NULL, vid, pid);

#ifdef PATCH_FOR_OLD_DEVICE
    if ((pid == USB_BM_PID_ROM) && (vid == USB_BM_VID) && (usb_handle == NULL))
    {
      usb_handle = libusb_open_device_with_vid_pid_mod(NULL, USB_BM_VID_OLD, USB_BM_PID_ROM);
    }
#endif

    if (usb_handle)
    {
      break;
    }

    usleep(200 * 1000);
  }

  while (1) {
    libusb_set_auto_detach_kernel_driver(usb_handle, 1);
    status = libusb_claim_interface(usb_handle, 0);
    if (status != 0) {
      gettimeofday(&end, NULL);
      if (((1000000 * (end.tv_sec-start.tv_sec)+ end.tv_usec-start.tv_usec) / (double)1000000) >= timeout) {
        break;
      } else {
        printf("device claim failed: %s. Retry\n", libusb_error_name(status));
      }

      usleep(50 * 1000);
      continue;
    }

    break;
  }

  RET_ERR_IF(status != 0, "device claim failed: %s", libusb_error_name(status));
  BM_PRINT("done\n");
  return status;
}

int close_usb_device(libusb_device_handle *usb_handle)
{
  //close device
  libusb_release_interface(usb_handle, 0);
  libusb_close(usb_handle);
  usb_handle = NULL;
  return 0;
}

int libusb_comm_open(void)
{
  int status;

  // init lib
  BM_PRINT("init libusb\n");
  status = libusb_init(0);
  RET_ERR_IF(status < 0, "usb init failed");

  // open usb
  status = open_usb_device(USB_BM_VID, USB_BM_PID_LINUX, 10);
  RET_ERR_IF(status != 0, "open usb failed");

  return status;
}

int libusb_comm_close(void)
{
    return close_usb_device(usb_handle);
}

int libusb_comm_read(void *data, size_t len, int *translen, unsigned int timeout_ms)
{
    int err = libusb_bulk_transfer(usb_handle, 0x81, (uint8_t*)data, len, translen, timeout_ms);
    RET_ERR_IF(err != 0, "read error:%d", err);
    return 0;
}

int libusb_comm_write(void *data, size_t len, int *translen, unsigned int timeout_ms)
{
    int len_get;
    int err = libusb_bulk_transfer(usb_handle, 0x01, (uint8_t*)data, len, translen, timeout_ms);
    RET_ERR_IF(err != 0, "write error:%d", err);
    return 0;
}


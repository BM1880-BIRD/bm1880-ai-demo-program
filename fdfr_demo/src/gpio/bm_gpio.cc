/*
********************************************************************
 * Demo program on Bitmain BM1880
 *
 * Copyright © 2018 Bitmain Technologies. All Rights Reserved.
 *
 * Author:  liang.wang02@bitmain.com
 *
 *******************************************************************
*/
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <queue>

using namespace std;

#include "bm_common_config.h"
#include "bm_gpio.h"


#define SYSFS_GPIO_DIR "/sys/class/gpio"
#define MAX_BUF 64

static int BmGpioExport(unsigned int gpio)
{
	int fd, len;
	char buf[MAX_BUF];

	fd = open(SYSFS_GPIO_DIR "/export", O_WRONLY);
	if (fd < 0) {
		perror("gpio/export");
		return fd;
	}

	len = snprintf(buf, sizeof(buf), "%d", gpio);
	write(fd, buf, len);
	close(fd);

	return 0;
}

int BmGpioUnexport(unsigned int gpio)
{
	int fd, len;
	char buf[MAX_BUF];

	fd = open(SYSFS_GPIO_DIR "/unexport", O_WRONLY);
	if (fd < 0) {
		perror("gpio/export");
		return fd;
	}

	len = snprintf(buf, sizeof(buf), "%d", gpio);
	write(fd, buf, len);
	close(fd);
	return 0;
}

static int BmGpioSetDirection(unsigned int gpio, unsigned int out_flag)
{
	int fd, len;
	char buf[MAX_BUF];

	len = snprintf(buf, sizeof(buf), SYSFS_GPIO_DIR"/gpio%d/direction", gpio);
	if (access(buf,0) == -1) {
		BmGpioExport(gpio);
	}

	fd = open(buf, O_WRONLY);
	if (fd < 0) {
		perror("gpio/direction");
		return fd;
	}

	if (out_flag)
		write(fd, "out", 4);
	else
		write(fd, "in", 3);

	close(fd);
	return 0;
}

int BmGpioSetValue(unsigned int gpio, unsigned int value)
{
	int fd, len;
	char buf[MAX_BUF];

	len = snprintf(buf, sizeof(buf),SYSFS_GPIO_DIR"/gpio%d/value", gpio);
	if (access(buf,0) == -1) {
		BM_LOG(LOG_DEBUG_ERROR, cout << buf << " not exist!" << endl);
		BmGpioSetDirection(gpio,1); //output
	}

	//len = snprintf(buf, sizeof(buf), SYSFS_GPIO_DIR "/gpio%d/value", gpio);

	fd = open(buf, O_WRONLY);
	if (fd < 0) {
		perror("gpio/set-value");
		return fd;
	}

	if (value) {
		write(fd, "1", 2);
	} else {
		write(fd, "0", 2);
	}

	close(fd);
	return 0;
}

int BmGpioGetValue(unsigned int gpio, unsigned int *value)
{
	int fd, len;
	char buf[MAX_BUF];
	char ch;

	len = snprintf(buf, sizeof(buf), SYSFS_GPIO_DIR "/gpio%d/value", gpio);

	if (access(buf,0) == -1) {
		BM_LOG(LOG_DEBUG_ERROR, cout << buf << " not exist!" << endl);
		BmGpioSetDirection(gpio,0); //output
	}

	fd = open(buf, O_RDONLY);
	if (fd < 0) {
		perror("gpio/get-value");
		return fd;
	}

	read(fd, &ch, 1);

	if (ch != '0') {
		*value = 1;
	} else {
		*value = 0;
	}

	close(fd);
	return 0;
}

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
	if(access(buf,0) == -1)
	{
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
	if(access(buf,0) == -1)
	{
		LOG(LOG_DEBUG_NORMAL,cout<<buf<<" not exist!"<<endl);
		BmGpioSetDirection(gpio,1); //output
	}
	
	//len = snprintf(buf, sizeof(buf), SYSFS_GPIO_DIR "/gpio%d/value", gpio);

	fd = open(buf, O_WRONLY);
	if (fd < 0) 
	{
		perror("gpio/set-value");
		return fd;
	}

	if (value)
	{
		write(fd, "1", 2);
	}
	else
	{
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

	if(access(buf,0) == -1)
	{
		LOG(LOG_DEBUG_NORMAL,cout<<buf<<" not exist!"<<endl);
		BmGpioSetDirection(gpio,0); //output
	}

	fd = open(buf, O_RDONLY);
	if (fd < 0)
	{
		perror("gpio/get-value");
		return fd;
	}

	read(fd, &ch, 1);

	if (ch != '0') 
	{
		*value = 1;
	}
	else
	{
		*value = 0;
	}

	close(fd);
	return 0;
}


#if 0
#define POLL_TIMEOUT (3 * 1000) /* 3 seconds */
int bm_gpio_set_edge(unsigned int gpio, char *edge)
{
	int fd, len;
	char buf[MAX_BUF];

	len = snprintf(buf, sizeof(buf), SYSFS_GPIO_DIR "/gpio%d/edge", gpio);

	fd = open(buf, O_WRONLY);
	if (fd < 0)
	{
		perror("gpio/set-edge");
		return fd;
	}

	write(fd, edge, strlen(edge) + 1); 
	close(fd);
	return 0;
}
 
int bm_gpio_fd_open(unsigned int gpio)
{
	int fd, len;
	char buf[MAX_BUF];
 
	len = snprintf(buf, sizeof(buf), SYSFS_GPIO_DIR "/gpio%d/value", gpio);
 
	fd = open(buf, O_RDONLY | O_NONBLOCK );
	if (fd < 0)
	{
		perror("gpio/fd_open");
	}
	return fd;
}

int bm_gpio_fd_close(int fd)
{
	return close(fd);
}

int main(int argc, char **argv, char **envp)
{
	struct pollfd fdset[2];
	int nfds = 2;
	int gpio_fd, timeout, rc;
	char *buf[MAX_BUF];
	unsigned int gpio;
	int len;

	if (argc < 2) {
		printf("Usage: gpio-int <gpio-pin>\n\n");
		printf("Waits for a change in the GPIO pin voltage level or input on stdin\n");
		exit(-1);
	}

	gpio = atoi(argv[1]);

	gpio_export(gpio);
	gpio_set_dir(gpio, 0);
	gpio_set_edge(gpio, "rising");
	gpio_fd = gpio_fd_open(gpio);

	timeout = POLL_TIMEOUT;

	while (1) {
		memset((void*)fdset, 0, sizeof(fdset));

		fdset[0].fd = STDIN_FILENO;
		fdset[0].events = POLLIN;

		fdset[1].fd = gpio_fd;
		fdset[1].events = POLLPRI;

		rc = poll(fdset, nfds, timeout);      

		if (rc < 0) {
			printf("\npoll() failed!\n");
			return -1;
		}

		if (rc == 0)
		{
			printf(".");
		}

		if (fdset[1].revents & POLLPRI)
		{
			len = read(fdset[1].fd, buf, MAX_BUF);
			printf("\npoll() GPIO %d interrupt occurred\n", gpio);
		}

		if (fdset[0].revents & POLLIN)
		{
			(void)read(fdset[0].fd, buf, 1);
			printf("\npoll() stdin read 0x%2.2X\n", (unsigned int) buf[0]);
		}

		fflush(stdout);
	}

	gpio_fd_close(gpio_fd);
	return 0;
}
#endif

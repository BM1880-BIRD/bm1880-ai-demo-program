/*
 ********************************************************************
 * Demo program on Bitmain BM1880
 *
 * Copyright © 2018 Bitmain Technologies. All Rights Reserved.
 *
 * Author:  liang.wang02@bitmain.com
 *
 ********************************************************************
*/
#include <iostream>
#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <fcntl.h>

using namespace std;

#define SYSFS_GPIO_DIR "/sys/class/gpio"
#define MAX_BUF 64

#define BM1880_GPIO(x) (x<32)?(480+x):((x<64)?(448+x-32):((x<=67)?(444+x-64):(-1)))

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
	if(access(buf,0) == -1) {
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
	if(access(buf,0) == -1) {
		cout << buf << " not exist!" << endl;
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
		cout << buf << " not exist!" << endl;
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


#define KEY_CLK		BM1880_GPIO(57)
#define KEY_DATA	BM1880_GPIO(63)
#define KEY_KB1		BM1880_GPIO(64)
#define KEY_KB2		BM1880_GPIO(65)


void Bm74Hc164Shift1Bit(int Value)
{
	BmGpioSetValue(KEY_CLK, 0);
	BmGpioSetValue(KEY_DATA, Value);
	BmGpioSetValue(KEY_CLK, 1);
}
void BmUf100KeyPadOutValue(unsigned char Value)
{
	unsigned char u1Val = 0x80;
	int i;

	for(i=0; i<8; i++) {
		//printf("u1Val = 0x%x .\n", u1Val);
		Bm74Hc164Shift1Bit(Value & u1Val);
		u1Val = u1Val >> 1;
	}
}

int BmUf100KeyPadGetKey()
{
	unsigned int KeyValue;
	int i;
	unsigned char u1Val;
	while(1) {
		BmGpioGetValue(KEY_KB1, &KeyValue);
		if(KeyValue == 0) {
			u1Val = 0x80;
			for(i=0; i<=8; i++) {
				//printf("xxx = 0x%X .\n",~u1Val);
				BmUf100KeyPadOutValue(~u1Val);
				BmGpioGetValue(KEY_KB1, &KeyValue);
				if(KeyValue == 0) {
					//cout << "Get KEY_1" << endl;
					break;
				}
				u1Val = u1Val >> 1;
			}
			switch(u1Val) {
				case 0x01:
					cout << "Get KEY_1" << endl;
					break;
				case 0x02:
					cout << "Get KEY_2" << endl;
					break;
				case 0x04:
					cout << "Get KEY_3" << endl;
					break;
				case 0x08:
					cout << "Get KEY_4" << endl;
					break;
				case 0x10:
					cout << "Get KEY_5" << endl;
					break;
				case 0x20:
					cout << "Get KEY_6" << endl;
					break;
				case 0x40:
					cout << "Get KEY_7" << endl;
					break;
				case 0x80:
					cout << "Get KEY_8" << endl;
					break;
				default:
					cout << "Get none" << endl;
					break;
			}
			BmUf100KeyPadOutValue(0x00);
		}
		BmGpioGetValue(KEY_KB2, &KeyValue);
		if(KeyValue == 0) {
			u1Val = 0x80;
			for(i=0; i<=8; i++) {
				BmUf100KeyPadOutValue(~u1Val);
				BmGpioGetValue(KEY_KB2, &KeyValue);
				if(KeyValue == 0) {
					//cout << "Get KEY_1" << endl;
					break;
				}
				u1Val = u1Val >> 1;
			}
			switch(u1Val) {
				case 0x01:
					cout << "Get KEY_9" << endl;
					break;
				case 0x02:
					cout << "Get KEY_10" << endl;
					break;
				case 0x04:
					cout << "Get KEY_11" << endl;
					break;
				case 0x08:
					cout << "Get KEY_12" << endl;
					break;
				case 0x10:
					cout << "Get KEY_13" << endl;
					break;
				case 0x20:
					cout << "Get KEY_14" << endl;
					break;
				case 0x40:
					cout << "Get KEY_15" << endl;
					break;
				case 0x80:
					cout << "Get KEY_16" << endl;
					break;
				default:
					cout << "Get none" << endl;
					break;
			}	
			BmUf100KeyPadOutValue(0x00);
		}
		usleep(100000);
	}
}

int main(int argc, char *argv[])
{
	if (argc == 1) {
		cout << "Usage: keypad [1]" << endl;
		return 0;
	} else //if(argc == 2)
	{
		BmGpioSetValue(KEY_CLK, 0);
		BmGpioSetValue(KEY_DATA, 0);
		BmUf100KeyPadOutValue(0x00);
		if((!strcmp(argv[1],"1"))) {
			BmUf100KeyPadGetKey();
		}
		return 0;
	}
	cout << "Input Error !!\n" << endl;
	return -1;
}


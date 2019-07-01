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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <getopt.h>

#define EDB_GPIO(x) (x<32)?(480+x):((x<64)?(448+x-32):((x<=67)?(444+x-64):(-1)))
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

	//printf("mark %d , %s \n",out_flag, buf);
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
		//BM_LOG(LOG_DEBUG_ERROR, cout << buf << " not exist!" << endl);
		BmGpioExport(gpio);
	}

	BmGpioSetDirection(gpio,1); //output

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
		//BM_LOG(LOG_DEBUG_ERROR, cout << buf << " not exist!" << endl);
		BmGpioExport(gpio);
	}

	BmGpioSetDirection(gpio,0); //input

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

void BmExt40List(void)
{
	printf("\n\
+---+---+---+---+---+BM1880 EDB+---+---+---+---+---+---+\n\
| Function Name     | Physical | Function Name         |\n\
+---+---+---+---+---+----++----+---+---+---+---+---+---+\n\
| GND               | 01 || 02 | GND                   |\n\
| GPIO55/UART13_CTS | 03 || 04 | PWR_BTN_N             |\n\
| GPIO48/UART13_TXD | 05 || 06 | RST_BTN_N             |\n\
| GPIO49/UART13_RXD | 07 || 08 | GPIO33/SPI0_SCK/PWM23 |\n\
| GPIO54/UART13_RTS | 09 || 10 | GPIO31/SPI0_SDI/PWM21 |\n\
| GPIO46            | 11 || 12 | GPIO30/SPI0_CS/PWM20  |\n\
| GPIO47            | 13 || 14 | GPIO32/SPI0_SDO/PWM22 |\n\
| GPIO35/I2C0_SCL   | 15 || 16 | GPIO59/I2S0_FS        |\n\
| GPIO34/I2C0_SDA   | 17 || 18 | GPIO58/I2S0_SCLK      |\n\
| GPIO37/I2C1_SCL   | 19 || 20 | GPIO61/I2S0_SDO       |\n\
| GPIO36/I2C1_SDA   | 21 || 22 | GPIO60/I2S0_SDI       |\n\
| GPIO0/I2S0_MCLKIN | 23 || 24 | GPIO62/I2S0_MCLKIN    |\n\
| GPIO1/I2S1_MCLKIN | 25 || 26 | GPIO64/I2S1_FS        |\n\
| GPIO3             | 27 || 28 | GPIO63/I2S1_SCLK      |\n\
| GPIO7             | 29 || 30 | GPIO66/I2S1_SDO       |\n\
| GPIO50/UART14_TXD | 31 || 32 | GPIO65/I2S1_SDI       |\n\
| GPIO51/UART14_RXD | 33 || 34 | GPIO67/I2S1_MCLKIN    |\n\
| 1V8               | 35 || 36 | 12V                   |\n\
| 5V                | 37 || 38 | 12V                   |\n\
| GND               | 39 || 40 | GND                   |\n\
+---+---+---+---+---+----++----+---+---+---+---+---+---+\n\n");

}

void BmPrintHelp(void)
{
	printf("\n\
Usage: \n\
list     | gpio --list             ; show ext 40pin list.\n\
help     | gpio --help             ; print this help and exit.\n\
out      | gpio --out [gpio] [val] ; set gpio high/low.\n\
in       | gpio --in [gpio]        ; get gpio high/low.\n\
release  | gpio --release          ; release gpio.\n\
\n");

}


static struct option long_options[] = \
{
	{"list",	no_argument,		NULL,	'l'},
	{"help",	no_argument,		NULL,	'h'},
	{"out",		required_argument,	NULL,	'o'},
	{"in",		required_argument,	NULL,	'i'},
	{"release",	required_argument,	NULL,	'r'},
	{"test",	required_argument,	NULL,	't'},
};


int main(int argc, char* argv[])
{
	int opt;
    int digit_optind = 0;
    int option_index = 0;
	int gpio_num = 0, gpio_hl = 0;

	if(argc == 1)
	{
		BmPrintHelp();
		return 0;
	}

	while((opt = getopt_long(argc, argv, "c:s:p", long_options, &option_index))!= -1)
    {
    	#if 0
        printf("opt = 0x%x\t\t", opt);
        printf("optarg = %s\t\t",optarg);
        printf("optind = %d\t\t",optind);
        printf("argv[optind] =%s\t\t", argv[optind]);
        printf("option_index = %d\n",option_index);
		#endif

		switch(opt)
		{
			case 'l':
				BmExt40List();
				break;
			case '?':
			case 'h':
				BmPrintHelp();
				break;
			case 'o':
				if(argv[optind] == NULL)
				{
					printf("invalid input!!");
				}
				else
				{
					gpio_num = atoi(optarg);
					gpio_hl = atoi(argv[optind]);
					BmGpioSetValue(EDB_GPIO(gpio_num), gpio_hl);
					printf("set GPIO[%d] = %d\n", gpio_num, gpio_hl);
				}
				break;
			case 'i':
				gpio_num = atoi(optarg);
				BmGpioGetValue(EDB_GPIO(gpio_num), &gpio_hl);
				printf("%d\n\n",gpio_hl);
				break;
			case 'r':
				gpio_num = atoi(optarg);
				BmGpioUnexport(EDB_GPIO(gpio_num));
				break;
			case 't':
				gpio_num = atoi(optarg);
				printf("%d\n\n",EDB_GPIO(gpio_num));
				break;
		}
	}


}

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
#include <sys/ioctl.h>
#include <sys/time.h>
#include <linux/ioctl.h>
#include <sys/stat.h>
#include <linux/types.h>
#include <linux/spi/spidev.h>
#include <linux/fb.h>
#include <fcntl.h>
#include <sys/mman.h>

using namespace std;

#include "bm_spi_lcd.h"
#include "bm_logo_data_hex.inc"


static const char *device = LCD_SPI_DEV;
static uint32_t mode = SPI_MODE_0;
static uint8_t bits = 8;
static char *input_file;
static char *output_file;
static uint32_t speed = 25000000;
static uint16_t delay = 0;
int fd_spidev;
int fbfd;
unsigned char *fbbuf;
struct fb_var_screeninfo fb_info;
uint8_t lcd_display_buf[LCD_COL*LCD_ROW*2];
struct spi_ioc_transfer tr;
//queue &signal for lcd display

static struct timeval start_time,end_time;


enum __lcd_display
{
	LCD_DRIVE_ST7789V = 0,
	LCD_DRIVE_ILI9341,
	FRAME_BUFFER_ENABLE
};

int lcd_type = LCD_DRIVE_ILI9341;


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



static void BmSpiTransfer(int fd, uint8_t const *tx, uint8_t const *rx, size_t len ,uint8_t bits)
{
	int ret;
	int out_fd;

	tr.tx_buf = (unsigned long)tx;
	tr.rx_buf = (unsigned long)rx;
	tr.len = len;
	tr.delay_usecs = delay;
	tr.speed_hz = speed;
	tr.bits_per_word = bits;

	ret = ioctl(fd, SPI_IOC_MESSAGE(1), &tr);
	if (ret < 1) {
		perror("can't send spi message");
	}
}

static void BmSpiSenddata(uint8_t data)
{
	BmSpiTransfer(fd_spidev, &data, NULL, sizeof(data), 8);
}

static void LcdWritecomm(uint8_t u1Cmd)
{
	BmGpioSetValue(LCD_RD_GPIO,0); //RD=0
	BmSpiSenddata(u1Cmd);
}

static void LcdWritedata(uint8_t u1Data)
{
	BmGpioSetValue(LCD_RD_GPIO,1); //RD=1
	BmSpiSenddata(u1Data);
}

static void LcdIli9341Init(void)
{
	//cout << __FUNCTION__ << " , " << __LINE__ << endl;
	BmGpioSetValue(LCD_RESET_GPIO, 1); //RESET=1
	usleep(200000);
	BmGpioSetValue(LCD_RESET_GPIO, 0); //RESET=0
	usleep(800000);
	BmGpioSetValue(LCD_RESET_GPIO, 1); //RESET=1
	usleep(800000);

	LcdWritecomm(0xCF);
	LcdWritedata(0x00);
	LcdWritedata(0xC1);
	LcdWritedata(0x30);

	LcdWritecomm(0xED);
	LcdWritedata(0x64);
	LcdWritedata(0x03);
	LcdWritedata(0x12);
	LcdWritedata(0x81);

	LcdWritecomm(0xE8);
	LcdWritedata(0x85);
	LcdWritedata(0x00);
	LcdWritedata(0x7A);

	LcdWritecomm(0xCB);
	LcdWritedata(0x39);
	LcdWritedata(0x2C);
	LcdWritedata(0x00);
	LcdWritedata(0x34);
	LcdWritedata(0x02);

	LcdWritecomm(0xF7);
	LcdWritedata(0x20);

	LcdWritecomm(0xEA);
	LcdWritedata(0x00);
	LcdWritedata(0x00);

	LcdWritecomm(0xc0);
	LcdWritedata(0x21);

	LcdWritecomm(0xc1);
	LcdWritedata(0x11);

	LcdWritecomm(0xc5);
	LcdWritedata(0x25);
	LcdWritedata(0x32);

	LcdWritecomm(0xc7);
	LcdWritedata(0xaa);

	LcdWritecomm(0x36);
	//LcdWritedata(0x08);
	LcdWritedata((1<<5)|(0<<6)|(1<<7)|(1<<3));

	LcdWritecomm(0xb6);
	LcdWritedata(0x0a);
	LcdWritedata(0xA2);

	LcdWritecomm(0xb1);
	LcdWritedata(0x00);
	LcdWritedata(0x1B);

	LcdWritecomm(0xf2);
	LcdWritedata(0x00);

	LcdWritecomm(0x26);
	LcdWritedata(0x01);

	LcdWritecomm(0x3a);
	LcdWritedata(0x55);

	LcdWritecomm(0xE0);
	LcdWritedata(0x0f);
	LcdWritedata(0x2D);
	LcdWritedata(0x0e);
	LcdWritedata(0x08);
	LcdWritedata(0x12);
	LcdWritedata(0x0a);
	LcdWritedata(0x3d);
	LcdWritedata(0x95);
	LcdWritedata(0x31);
	LcdWritedata(0x04);
	LcdWritedata(0x10);
	LcdWritedata(0x09);
	LcdWritedata(0x09);
	LcdWritedata(0x0d);
	LcdWritedata(0x00);

	LcdWritecomm(0xE1);
	LcdWritedata(0x00);
	LcdWritedata(0x12);
	LcdWritedata(0x17);
	LcdWritedata(0x03);
	LcdWritedata(0x0d);
	LcdWritedata(0x05);
	LcdWritedata(0x2c);
	LcdWritedata(0x44);
	LcdWritedata(0x41);
	LcdWritedata(0x05);
	LcdWritedata(0x0f);
	LcdWritedata(0x0a);
	LcdWritedata(0x30);
	LcdWritedata(0x32);
	LcdWritedata(0x0F);

	LcdWritecomm(0x11);
	usleep(120000);

	LcdWritecomm(0x29);
}

static void LcdSt7789vInit(void)
{
	BmGpioSetValue(LCD_RESET_GPIO,1); //RESET=1
	usleep(1000);  //delay 1ms
	BmGpioSetValue(LCD_RESET_GPIO,0); //RESET=0
	usleep(10000);  //delay 10ms
	BmGpioSetValue(LCD_RESET_GPIO,1); //RESET=1
	usleep(120000);  //delay 120ms

	//Display Setting
	LcdWritecomm(0x36);
	LcdWritedata(0x00);
	//LcdWritedata(0xE0);

	LcdWritecomm(0x3a);
	LcdWritedata(0x55);
	//ST7789V Frame rate setting
	LcdWritecomm(0xb2);
	LcdWritedata(0x0c);
	LcdWritedata(0x0c);
	LcdWritedata(0x00);
	LcdWritedata(0x33);
	LcdWritedata(0x33);
	LcdWritecomm(0xb7);
	LcdWritedata(0x35); //VGH=13V, VGL=-10.4V
	//--------------------------
	LcdWritecomm(0xbb);
	LcdWritedata(0x19);
	LcdWritecomm(0xc0);
	LcdWritedata(0x2c);
	LcdWritecomm(0xc2);
	LcdWritedata(0x01);
	LcdWritecomm(0xc3);
	LcdWritedata(0x12);
	LcdWritecomm(0xc4);
	LcdWritedata(0x20);
	LcdWritecomm(0xc6);
	LcdWritedata(0x0f);
	LcdWritecomm(0xd0);
	LcdWritedata(0xa4);
	LcdWritedata(0xa1);
	//--------------------------
	LcdWritecomm(0xe0); //gamma setting
	LcdWritedata(0xd0);
	LcdWritedata(0x04);
	LcdWritedata(0x0d);
	LcdWritedata(0x11);
	LcdWritedata(0x13);
	LcdWritedata(0x2b);
	LcdWritedata(0x3f);
	LcdWritedata(0x54);
	LcdWritedata(0x4c);
	LcdWritedata(0x18);
	LcdWritedata(0x0d);
	LcdWritedata(0x0b);
	LcdWritedata(0x1f);
	LcdWritedata(0x23);
	LcdWritecomm(0xe1);
	LcdWritedata(0xd0);
	LcdWritedata(0x04);
	LcdWritedata(0x0c);
	LcdWritedata(0x11);
	LcdWritedata(0x13);
	LcdWritedata(0x2c);
	LcdWritedata(0x3f);
	LcdWritedata(0x44);
	LcdWritedata(0x51);
	LcdWritedata(0x2f);
	LcdWritedata(0x1f);
	LcdWritedata(0x1f);
	LcdWritedata(0x20);
	LcdWritedata(0x23);
	LcdWritecomm(0x11);
	usleep(120000);
	LcdWritecomm(0x29); //display on
}


static void LcdBlockWrite(unsigned int Xstart,unsigned int Xend,unsigned int Ystart,unsigned int Yend)
{
	LcdWritecomm(0x2A);
	LcdWritedata(Xstart>>8);
	LcdWritedata(Xstart);
	LcdWritedata(Xend>>8);
	LcdWritedata(Xend);

	LcdWritecomm(0x2B);
	LcdWritedata(Ystart>>8);
	LcdWritedata(Ystart);
	LcdWritedata(Yend>>8);
	LcdWritedata(Yend);

	LcdWritecomm(0x2C);
}

static void BmLcdDisplayPicture(uint8_t *picture)
{
	if(lcd_type == FRAME_BUFFER_ENABLE) {
		memcpy(fbbuf,picture,320*240*2);

		if(ioctl(fbfd,FBIOPAN_DISPLAY,&fb_info) == -1) {
			BM_LOG(LOG_DEBUG_ERROR,cout << "FBIOPAN_DISPLAY error" << endl);
		}
	} else if((lcd_type == LCD_DRIVE_ST7789V) || (lcd_type == LCD_DRIVE_ILI9341)) {
		uint8_t *p;
		int i;
		int ret = 0;

		LcdBlockWrite(0,LCD_COL-1,0,LCD_ROW-1);
		BmGpioSetValue(LCD_RD_GPIO,1); //RD=1
		p = picture;

		//bits per word
		bits = 16;
		ret = ioctl(fd_spidev, SPI_IOC_WR_BITS_PER_WORD, &bits);
		if (ret == -1) {
			BM_LOG(LOG_DEBUG_ERROR,cout << "can't set bits per word" << endl);
		}

		#if 1
		for (i=0; i< (LCD_COL*LCD_ROW*2)/4096; i++) {
			write(fd_spidev, (p + i*4096), 4096);
		}
		write(fd_spidev, (p + i*4096), (320*240*2)%4096);
		#else
		write(fd_spidev, p, (320*240*2));
		#endif
	}
}


int BmFrameBufferInit(void)
{
	int fbsize;

	if ((fbfd = open("/dev/fb0", O_RDWR)) < 0) {
        printf("open fb0 failed\n");
        return -1;
    }
    if(ioctl(fbfd,FBIOGET_VSCREENINFO,&fb_info) == -1) {
        printf("zhxjun get screen info error\n");
        return -1;
    }

    fbsize = 320*240*2*2;
    fbbuf = (unsigned char*)mmap(0, fbsize, PROT_READ | PROT_WRITE, MAP_SHARED, fbfd, 0);
    if (fbbuf == MAP_FAILED) {
        printf("map video error.\n");
        return -1;
    }

    fb_info.xoffset = 0;
    fb_info.yoffset = 0;

	return 0;
}

int BmLcdInit(void)
{
	int ret = 0;
	uint8_t *p_str;

	fd_spidev= open(device, O_RDWR);
	if (fd_spidev < 0) {
		perror("can't open device");
		return fd_spidev;
	}
	//spi mode
	ret = ioctl(fd_spidev, SPI_IOC_WR_MODE32, &mode);
	if (ret == -1) {
		perror("can't set spi mode");
		return ret;
	}

	ret = ioctl(fd_spidev, SPI_IOC_RD_MODE32, &mode);
	if (ret == -1) {
		perror("can't get spi mode"); 
		return ret;
	}
	//bits per word
	ret = ioctl(fd_spidev, SPI_IOC_WR_BITS_PER_WORD, &bits);
	if (ret == -1) {
		perror("can't set bits per word");
		return ret;
	}

	ret = ioctl(fd_spidev, SPI_IOC_RD_BITS_PER_WORD, &bits);
	if (ret == -1) {
		perror("can't get bits per word");
		return ret;
	}

	//max speed hz
	ret = ioctl(fd_spidev, SPI_IOC_WR_MAX_SPEED_HZ, &speed);
	if (ret == -1) {
		perror("can't set max speed hz");
		return ret;
	}

	ret = ioctl(fd_spidev, SPI_IOC_RD_MAX_SPEED_HZ, &speed);
	if (ret == -1) {
		perror("can't get max speed hz");
		return ret;
	}
	BM_LOG(LOG_DEBUG_USER_(3),cout<<"spi mode: 0x"<<hex<<mode<<endl);
	BM_LOG(LOG_DEBUG_USER_(3),cout<<"max speed:"<<speed<<"Hz "<<"\"("<<(speed/1000)<<" KHz)\""<<endl);

	if(lcd_type == LCD_DRIVE_ST7789V) {
		LcdSt7789vInit();
	}
	else if(lcd_type == LCD_DRIVE_ILI9341) {
		LcdIli9341Init();
	}
	else if(lcd_type == FRAME_BUFFER_ENABLE) {
		if(BmFrameBufferInit() ==-1) {
			cout<<"FrameBuffer init fail"<<endl;
			return -1;
		}
	} else {
		BM_LOG(LOG_DEBUG_ERROR, cout<<"Error: invalid lcd type."<<endl);
		return -1;
	}
	
	return ret;
}





void DispColor(unsigned int color)
{
	unsigned int i;
	uint32_t *p;
	
	p = (uint32_t *)malloc(LCD_COL * LCD_ROW * 2);
	if (!p)
		perror("malloc failed.");
	
	for(i=0; i< LCD_COL * LCD_ROW/2; i++) {
		*(p + i) = (color << 16) | color;
	}
	
	BmLcdDisplayPicture((uint8_t *)p);
	free(p);
}

void DispBand(void)
{
	unsigned int i,j,k;
	uint32_t *p,*s;
	
	//unsigned int color[8]={0x001f,0x07e0,0xf800,0x07ff,0xf81f,0xffe0,0x0000,0xffff};
	uint16_t color[8]={RED,BLACK,GREEN,GRAY,GRAY25,BLUE,WHITE,RED};//0x94B2

	s = p = (uint32_t *)malloc(LCD_COL * LCD_ROW * 2);
	memset(p,0xFF,LCD_COL * LCD_ROW * 2);
	if (!p)
		perror("malloc failed.");

	
	for (i=0; i<8; i++) {
		for(j=0; j<LCD_ROW/8; j++) {
	        for(k=0; k<LCD_COL/2; k++) {
				*(p++) = (color[i] << 16) | color[i];
			}
		}
	}
	
	BmLcdDisplayPicture((uint8_t *)s);
	
	free(s);
}

int main(int argc, char *argv[])
{
	if (argc == 1) {
		cout << "Usage: lcd [pic/red/band]" << endl;
		return 0;
	} else //if(argc == 2)
	{
		BmLcdInit();
		if((!strcmp(argv[1],"pic"))) {
			BmLcdDisplayPicture(logo_hex_8);
			cout << "lcd_display : " << endl;
			//return 0;
		}
		else if ((!strcmp(argv[1],"red"))) {
			DispColor(RED);
		}
		else if ((!strcmp(argv[1],"band"))) {
			DispBand();
		}
		close(fd_spidev);
		return 0;
	}
	cout << "Input Error !!\n" << endl;
	return -1;
}


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
#include <opencv2/videoio.hpp>
#include <opencv2/videoio/videoio_c.h>
#include <opencv2/core/core.hpp>
#include <opencv2/opencv.hpp>
using namespace std;
using namespace cv;

#include "bm_common_config.h"
#include "bm_spi_lcd.h"
#include "bm_gpio.h"
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
std::queue<cv::Mat>lcd_imagebuffer;
std::condition_variable lcd_available_;
std::mutex lcd_lock_;
pthread_t lcd_tid;


enum __lcd_display
{
	LCD_DRIVE_ST7789V = 0,
	LCD_DRIVE_ILI9341,
	FRAME_BUFFER_ENABLE
};

#ifdef CUSTOM_ZKT
int lcd_type = LCD_DRIVE_ST7789V;
#else
int lcd_type = LCD_DRIVE_ILI9341;
#endif


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


#if 0
void DispColor(unsigned short color)
{
	unsigned int i,j,k=0;

	LcdBlockWrite(0,LCD_COL-1,0,LCD_ROW-1);

	//system("echo 1 > /sys/class/gpio/gpio487/value");//RS=1;
	BmGpioSetValue(LCD_RD_GPIO,1); //RD=1

	for(i=0;i<LCD_ROW;i++)
	{
	    for(j=0;j<LCD_COL;j++)
		{
			BmSpiSenddata(color>>8);
			BmSpiSenddata(color);
			//default_tx[k++] = color;
			//default_tx[k++] = color;
		}
	}

	//transfer(fd_spidev, default_tx, default_rx, sizeof(default_tx));
}

void DispBand(void)
{
	unsigned int i,j,k;
	//unsigned int color[8]={0x001f,0x07e0,0xf800,0x07ff,0xf81f,0xffe0,0x0000,0xffff};
	unsigned int color[8]={0xf800,0xf800,0x07e0,0x07e0,0x001f,0x001f,0xffff,0xffff};//0x94B2
	//unsigned int gray16[]={0x0000,0x1082,0x2104,0x3186,0x42,0x08,0x528a,0x630c,0x738e,0x7bcf,0x9492,0xa514,0xb596,0xc618,0xd69a,0xe71c,0xffff};

   	BlockWrite(0,LCD_COL-1,0,LCD_ROW-1);

	//CS0=0;
	//RD0=1;
	system("echo 1 > /sys/class/gpio/gpio487/value");//RS=1;

	for(i=0;i<8;i++)
	{
		for(j=0;j<LCD_ROW/8;j++)
		{
	        for(k=0;k<LCD_COL;k++)
			{
				bm_lcd_SendDataSPI(color[i]>>8);
				bm_lcd_SendDataSPI(color[i]);

			}
		}
	}
	for(j=0;j<LCD_ROW%8;j++)
	{
        for(k=0;k<LCD_COL;k++)
		{

			bm_lcd_SendDataSPI(color[7]>>8);
			//bm_lcd_SendDataSPI(DBL=color[7]);
			bm_lcd_SendDataSPI(color[7]);

		}
	}

	//CS0=1;
}
#endif

static void BmLcdDisplayPicture(uint8_t *picture)
{
	if (lcd_type == FRAME_BUFFER_ENABLE) {
		memcpy(fbbuf,picture,320*240*2);

		if(ioctl(fbfd,FBIOPAN_DISPLAY,&fb_info) == -1) {
			BM_LOG(LOG_DEBUG_ERROR,cout << "FBIOPAN_DISPLAY error" << endl);
		}
	}
	else if((lcd_type == LCD_DRIVE_ST7789V) || (lcd_type == LCD_DRIVE_ILI9341))
	{
		uint8_t *p;
		int i;
		int ret = 0;

		LcdBlockWrite(0,LCD_COL-1,0,LCD_ROW-1);
		BmGpioSetValue(LCD_RD_GPIO,1); //RD=1
		p = picture;



		//bits per word
		bits = 16;
		ret = ioctl(fd_spidev, SPI_IOC_WR_BITS_PER_WORD, &bits);
		if (ret == -1)
		{
			BM_LOG(LOG_DEBUG_ERROR,cout << "can't set bits per word" << endl);
		}

		#if 0
		for(i=0; i< (LCD_COL*LCD_ROW*2)/4096; i++)
		{
			BmSpiTransfer(fd_spidev, (p + i*4096), NULL, 4096, 16);
		}
		BmSpiTransfer(fd_spidev, (p + i*4096), NULL, (320*240*2)%4096, 16);
		#endif

		#if 1
		for(i=0; i< (LCD_COL*LCD_ROW*2)/4096; i++) {
			write(fd_spidev, (p + i*4096), 4096);
		}
		write(fd_spidev, (p + i*4096), (320*240*2)%4096);
		#else
		write(fd_spidev, p, (320*240*2));
		#endif
	}
}

void* BmLcdThread(void *arg)
{
	while (true) {
		std::unique_lock<std::mutex> locker(lcd_lock_);
		lcd_available_.wait(locker);

		if (!lcd_imagebuffer.empty()) {
			auto image = std::move(lcd_imagebuffer.front());
			lcd_imagebuffer.pop();
			locker.unlock();
			BM_LOG(LOG_DEBUG_USER_(3),cout<<"start to send frame to lcd\n");

			BmLcdDisplayFrame(image);
			BM_LOG(LOG_DEBUG_USER_(3),cout<<"send done\n");
			//usleep(50000);
		}
	}
}

int BmFrameBufferInit(void)
{
	int fbsize;

	if ((fbfd = open("/dev/fb0", O_RDWR)) < 0) {
		printf("open fb0 failed\n");
		return -1;
	}

	if (ioctl(fbfd,FBIOGET_VSCREENINFO,&fb_info) == -1) {
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

	int fd;
	fd = open("/sys/devices/platform/spi0-mux/bm_mux", O_WRONLY);
	if (fd < 0) {
		perror("spi0-mux/bm_mux");
		return fd;
	}
	write(fd, "1", 1);
	close(fd);

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
	BM_LOG(LOG_DEBUG_USER_(3), cout<<"spi mode: 0x"<<hex<<mode<<endl);
	BM_LOG(LOG_DEBUG_USER_(3),
		cout<<"max speed:"<<speed<<"Hz "<<"\"("<<(speed/1000)<<" KHz)\""<<endl);

	if (lcd_type == LCD_DRIVE_ST7789V) {
		LcdSt7789vInit();
		//DispBand();
		//BmLcdDisplayPicture((uint8_t *)logo_hex_16);
		//LcdWritecomm(0x36);
		//LcdWritedata(0xE0);
	} else if(lcd_type == LCD_DRIVE_ILI9341) {
		LcdIli9341Init();
	} else if(lcd_type == FRAME_BUFFER_ENABLE) {
		if (BmFrameBufferInit() ==-1) {
			cout<<"FrameBuffer init fail"<<endl;
			return -1;
		}
	} else {
		BM_LOG(LOG_DEBUG_ERROR, cout<<"Error: invalid lcd type."<<endl);
		return -1;
	}

	pthread_create(&lcd_tid,NULL,BmLcdThread,NULL);
	//DispColor(RED);
	//close(fd_spidev);
	return ret;
}

static struct timeval start_time,end_time;
void BmLcdDisplayFrame(const cv::Mat &frame)
{
	cv::Mat BGR565;
	cv::cvtColor(frame,BGR565,cv::COLOR_BGR2BGR565);
	BmLcdDisplayPicture(BGR565.data);

	PERFORMACE_TIME_MESSURE_END(end_time);
	PERFORMACE_TIME_MESSURE_START(start_time);

#if 0
	int rowNumber = srcImage.rows;
	int colNumber = srcImage.cols;
	int i,j;
	for (i = 0; i < rowNumber; i++)
	{
		for (j= 0; j < colNumber; j++)
		{
			unsigned short B = srcImage.at<Vec3b>(i, j)[0]; //B
			unsigned short G = srcImage.at<Vec3b>(i, j)[1]; //G
			unsigned short R = srcImage.at<Vec3b>(i, j)[2]; //R
			//hex 8
			unsigned short pix_data = ((R << 8) & 0xF800) | ((G << 3) & 0x07E0) | ((B >> 3) & 0x001F);
			lcd_display_buf[i*colNumber*2 + j*2 ] = (char)((pix_data >> 8) & 0xFF);
			lcd_display_buf[i*colNumber*2 + j*2 + 1] = (char)(pix_data & 0xFF);
		}
	}
	//memcpy(lcd_display_buf,BGR565.data,320*240*2);
	//BmLcdDisplayPicture(lcd_display_buf);
#endif
}



#define LCD_C 320
#define LCD_R 240

void BmLcdAddDisplayFrame(const cv::Mat &frame)
{
	cv::Mat img_resize;
	if(edb_config.lcd_resize_type == 0) {
		cv::resize(frame, img_resize, Size(320, 240) );
	} else if (edb_config.lcd_resize_type == 1) {
		cv::Mat tmp,img_tmp(LCD_R, LCD_C, CV_8UC3,Scalar(0,0,0));
		int x = LCD_C;
		int y = LCD_R;
		int scale_c = (frame.cols/32);
		int scale_r = (frame.rows/24);
		if (scale_c > scale_r) {
			//x = LCD_C;
			y = (frame.rows*10/scale_c);
		} else {
			x = (frame.cols*10/scale_r);
			//y = LCD_R;
		}
		cv::resize(frame, tmp,  Size(x, y) );
		//cout<<"imageResize size : cols = "<<tmp.cols<<", rows = "<<tmp.rows<<endl;
		cv::Mat imageROI = img_tmp(cv::Rect((320 - tmp.cols)/2,(240 - tmp.rows)/2,tmp.cols,tmp.rows));
		tmp.copyTo(imageROI);
		img_resize = img_tmp;
	}

	std::lock_guard<std::mutex> locker(lcd_lock_);
	lcd_imagebuffer.push(img_resize);
	if (lcd_imagebuffer.size() > 3)
	{
		lcd_imagebuffer.pop();
		BM_LOG(LOG_DEBUG_USER_(3),cout << "queue full" << endl);
	}
	lcd_available_.notify_one();
	BM_LOG(LOG_DEBUG_USER_(3),cout << "queue size : " << lcd_imagebuffer.size() << endl);
}


int CliCmdEnableLcd(int argc, char *argv[])
{
	int level;
	if (argc == 1) {
		cout << "Usage: lcd enable" << endl;
		return 0;
	} else //if(argc == 2)
	{
		if((!strcmp(argv[1],"enable")))
		{
			edb_config.lcd_display = true;
		    BmLcdInit();
			BmLcdDisplayPicture(logo_hex_8);

			cout << "lcd_display : " << edb_config.lcd_display << endl;
			//return 0;
		}
		if((!strcmp(argv[2],"speed")))
		{
			speed = atoi(argv[3]);
		}
		return 0;
	}
	cout << "Input Error !!\n" << endl;
	return -1;
}


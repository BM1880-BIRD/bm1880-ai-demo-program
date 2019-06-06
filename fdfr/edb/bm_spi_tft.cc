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
#include <opencv2/videoio.hpp>
#include <opencv2/videoio/videoio_c.h>
#include <opencv2/core/core.hpp>
#include <opencv2/opencv.hpp>
using namespace std;
using namespace cv;

#include "bm_common_config.h"
#include "bm_spi_tft.h"
#include "bm_gpio.h"
#include "bm_logo_data_hex.inc"


static const char *device = TFT_SPI_DEV;
static uint32_t mode = SPI_MODE_0;
static uint8_t bits = 8;
static char *input_file;
static char *output_file;
static uint32_t speed = 25000000;
static uint16_t delay = 0;
int fd_spidev;
uint8_t tft_display_buf[TFT_COL*TFT_ROW*2];
struct spi_ioc_transfer tr;
bool tft_enable = false;
//queue &signal for tft display
std::queue<cv::Mat>tft_imagebuffer;
std::condition_variable tft_available_;
std::mutex tft_lock_;
pthread_t tft_tid;

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
	if (ret < 1)
	{
		perror("can't send spi message");
	}
}

static void BmSpiSenddata(uint8_t data)
{
	BmSpiTransfer(fd_spidev, &data, NULL, sizeof(data), 8);
}

static void TftSt7789vWritecomm(uint8_t u1Cmd)
{
	BmGpioSetValue(TFT_RD_GPIO,0); //RD=0
	BmSpiSenddata(u1Cmd);
}

static void TftSt7789vWritedata(uint8_t u1Data)
{
	BmGpioSetValue(TFT_RD_GPIO,1); //RD=1
	BmSpiSenddata(u1Data);
}

#define LcdWritecomm TftSt7789vWritecomm
#define LcdWritedata TftSt7789vWritedata
#define LCD_RESET_GPIO TFT_RESET_GPIO


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

static void TftSt7789vInit(void)
{
	BmGpioSetValue(TFT_RESET_GPIO,1); //RESET=1
	usleep(1000);  //delay 1ms
	BmGpioSetValue(TFT_RESET_GPIO,0); //RESET=0
	usleep(10000);  //delay 10ms
	BmGpioSetValue(TFT_RESET_GPIO,1); //RESET=1
	usleep(120000);  //delay 120ms
	
	//Display Setting
	TftSt7789vWritecomm(0x36);
	//TftSt7789vWritedata(0x00);
	//TftSt7789vWritedata(0xA0);
	TftSt7789vWritedata(0x20);
	//printf("lcd_cmd_36h = 0x%X \n",lcd_cmd_36h);

	TftSt7789vWritecomm(0x3a);
	TftSt7789vWritedata(0x55);
	//ST7789V Frame rate setting
	TftSt7789vWritecomm(0xb2);
	TftSt7789vWritedata(0x0c);
	TftSt7789vWritedata(0x0c);
	TftSt7789vWritedata(0x00);
	TftSt7789vWritedata(0x33);
	TftSt7789vWritedata(0x33);
	TftSt7789vWritecomm(0xb7);
	TftSt7789vWritedata(0x35); //VGH=13V, VGL=-10.4V
	//--------------------------
	TftSt7789vWritecomm(0xbb);
	TftSt7789vWritedata(0x19);
	TftSt7789vWritecomm(0xc0);
	TftSt7789vWritedata(0x2c);
	TftSt7789vWritecomm(0xc2);
	TftSt7789vWritedata(0x01);
	TftSt7789vWritecomm(0xc3);
	TftSt7789vWritedata(0x12);
	TftSt7789vWritecomm(0xc4);
	TftSt7789vWritedata(0x20);
	TftSt7789vWritecomm(0xc6);
	TftSt7789vWritedata(0x0f);
	TftSt7789vWritecomm(0xd0);
	TftSt7789vWritedata(0xa4);
	TftSt7789vWritedata(0xa1);
	//--------------------------
	TftSt7789vWritecomm(0xe0); //gamma setting
	TftSt7789vWritedata(0xd0);
	TftSt7789vWritedata(0x04);
	TftSt7789vWritedata(0x0d);
	TftSt7789vWritedata(0x11);
	TftSt7789vWritedata(0x13);
	TftSt7789vWritedata(0x2b);
	TftSt7789vWritedata(0x3f);
	TftSt7789vWritedata(0x54);
	TftSt7789vWritedata(0x4c);
	TftSt7789vWritedata(0x18);
	TftSt7789vWritedata(0x0d);
	TftSt7789vWritedata(0x0b);
	TftSt7789vWritedata(0x1f);
	TftSt7789vWritedata(0x23);
	TftSt7789vWritecomm(0xe1);
	TftSt7789vWritedata(0xd0);
	TftSt7789vWritedata(0x04);
	TftSt7789vWritedata(0x0c);
	TftSt7789vWritedata(0x11);
	TftSt7789vWritedata(0x13);
	TftSt7789vWritedata(0x2c);
	TftSt7789vWritedata(0x3f);
	TftSt7789vWritedata(0x44);
	TftSt7789vWritedata(0x51);
	TftSt7789vWritedata(0x2f);
	TftSt7789vWritedata(0x1f);
	TftSt7789vWritedata(0x1f);
	TftSt7789vWritedata(0x20);
	TftSt7789vWritedata(0x23);
	TftSt7789vWritecomm(0x11);
	usleep(120000);
	TftSt7789vWritecomm(0x29); //display on
}

static void TftSt7789vBlockWrite(unsigned int Xstart,unsigned int Xend,unsigned int Ystart,unsigned int Yend)
{
	TftSt7789vWritecomm(0x2A);             
	TftSt7789vWritedata(Xstart>>8);             
	TftSt7789vWritedata(Xstart);             
	TftSt7789vWritedata(Xend>>8);             
	TftSt7789vWritedata(Xend);             
	
	TftSt7789vWritecomm(0x2B);             
	TftSt7789vWritedata(Ystart>>8);             
	TftSt7789vWritedata(Ystart);             
	TftSt7789vWritedata(Yend>>8);             
	TftSt7789vWritedata(Yend);   
	
	TftSt7789vWritecomm(0x2C);
}


#if 0
void DispColor(unsigned short color)
{
	unsigned int i,j,k=0;

	TftSt7789vBlockWrite(0,TFT_COL-1,0,TFT_ROW-1);

	//system("echo 1 > /sys/class/gpio/gpio487/value");//RS=1;
	BmGpioSetValue(TFT_RD_GPIO,1); //RD=1

	for(i=0;i<TFT_ROW;i++)
	{
	    for(j=0;j<TFT_COL;j++)
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

   	BlockWrite(0,TFT_COL-1,0,TFT_ROW-1);
    	
	//CS0=0;
	//RD0=1;
	system("echo 1 > /sys/class/gpio/gpio487/value");//RS=1; 

	for(i=0;i<8;i++)
	{
		for(j=0;j<TFT_ROW/8;j++)
		{
	        for(k=0;k<TFT_COL;k++)
			{
				bm_tft_SendDataSPI(color[i]>>8); 
				bm_tft_SendDataSPI(color[i]);  

			} 
		}
	}
	for(j=0;j<TFT_ROW%8;j++)
	{
        for(k=0;k<TFT_COL;k++)
		{
			    
			bm_tft_SendDataSPI(color[7]>>8);  
			//bm_tft_SendDataSPI(DBL=color[7]);
			bm_tft_SendDataSPI(color[7]); 

		} 
	}
	
	//CS0=1;
}
#endif

static void BmTftDisplayPicture(uint8_t *picture)
{
	uint8_t *p;
	int i;
	int ret = 0;
	
	TftSt7789vBlockWrite(0,TFT_COL-1,0,TFT_ROW-1);
	BmGpioSetValue(TFT_RD_GPIO,1); //RD=1
	p = picture;

	#if 0
	for(i=0; i< (TFT_COL*TFT_ROW*2)/4096; i++)
	{
		BmSpiTransfer(fd_spidev, (p + i*4096), NULL, 4096, 16);
	}
	BmSpiTransfer(fd_spidev, (p + i*4096), NULL, (320*240*2)%4096, 16);
	#else
	//BmSpiTransfer(fd_spidev, p, NULL, (320*240*2), 16);
	//cout<<"BmSpiTransfer "<<(320*240*2)<<endl;

	//bits per word
	bits = 16;
	ret = ioctl(fd_spidev, SPI_IOC_WR_BITS_PER_WORD, &bits);
	if (ret == -1)
	{
		perror("can't set bits per word");
	}
	#if 1
	for (i=0; i< (320*240*2)/4096; i++) {
		write(fd_spidev, (p + i*4096), 4096);
	}
	write(fd_spidev, (p + i*4096), (320*240*2)%4096);
	#else
	write(fd_spidev, p, (320*240*2));
	#endif

	#endif
}

int BmTftInit(void)
{
	int ret = 0;
	uint8_t *p_str;

	fd_spidev= open(device, O_RDWR);
	if (fd_spidev < 0)
	{
		perror("can't open device");
		return fd_spidev;
	}
	//spi mode
	ret = ioctl(fd_spidev, SPI_IOC_WR_MODE32, &mode);
	if (ret == -1)
	{
		perror("can't set spi mode");
		return ret;
	}

	ret = ioctl(fd_spidev, SPI_IOC_RD_MODE32, &mode);
	if (ret == -1)
	{
		perror("can't get spi mode");
		return ret;
	}
	//bits per word
	ret = ioctl(fd_spidev, SPI_IOC_WR_BITS_PER_WORD, &bits);
	if (ret == -1)
	{
		perror("can't set bits per word");
		return ret;
	}

	ret = ioctl(fd_spidev, SPI_IOC_RD_BITS_PER_WORD, &bits);
	if (ret == -1)
	{
		perror("can't get bits per word");
		return ret;
	}

	//max speed hz
	ret = ioctl(fd_spidev, SPI_IOC_WR_MAX_SPEED_HZ, &speed);
	if (ret == -1)
	{
		perror("can't set max speed hz");
		return ret;
	}

	ret = ioctl(fd_spidev, SPI_IOC_RD_MAX_SPEED_HZ, &speed);
	if (ret == -1)
	{
		perror("can't get max speed hz");
		return ret;
	}
	LOG(LOG_DEBUG_NORMAL,cout<<"spi mode: 0x"<<hex<<mode<<endl);
	//LOG(LOG_DEBUG_NORMAL,cout<<"bits per word: "<<bits<<endl);
	LOG(LOG_DEBUG_NORMAL,cout<<"max speed:"<<speed<<"Hz "<<"\"("<<(speed/1000)<<" KHz)\""<<endl);
	
	//TftSt7789vInit();
	LcdIli9341Init();
	//DispBand();

	//BmTftDisplayPicture((uint8_t *)logo_hex_16);
	BmTftDisplayPicture(logo_hex_8);

	//DispColor(RED);
	//close(fd_spidev);
	return ret;
}

#define PERF_TEST

#ifdef PERF_TEST
static struct timeval start_time,end_time;
static float cap_cost;
#endif

void BmTftDisplayFrame(const cv::Mat &frame)
{
	cv::Mat BGR565;
	cv::cvtColor(frame,BGR565,cv::COLOR_BGR2BGR565);
	BmTftDisplayPicture(BGR565.data);

#ifdef PERF_TEST
	gettimeofday(&end_time,NULL);
	cap_cost = (end_time.tv_sec - start_time.tv_sec)*1000000.0 + end_time.tv_usec-start_time.tv_usec;
	cout<<"BmTftDisplayFrame costtime: "<<cap_cost<<" us."<<endl;
	gettimeofday(&start_time,NULL);
#endif
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
			tft_display_buf[i*colNumber*2 + j*2 ] = (char)((pix_data >> 8) & 0xFF); 
			tft_display_buf[i*colNumber*2 + j*2 + 1] = (char)(pix_data & 0xFF); 
		}
	}
	//memcpy(tft_display_buf,BGR565.data,320*240*2);
	//BmTftDisplayPicture(tft_display_buf);
#endif
}

void* BmTftThread(void *arg)
{
	while(true)
	{
		std::unique_lock<std::mutex> locker(tft_lock_);
		tft_available_.wait(locker);
		
		if(!tft_imagebuffer.empty())
		{
			auto image = std::move(tft_imagebuffer.front());
			tft_imagebuffer.pop();
			locker.unlock();
			LOG(LOG_DEBUG_USER_4,"start to send frame to tft\n");
			BmTftDisplayFrame(image);
			LOG(LOG_DEBUG_USER_4,"send done\n");
			//usleep(50000);
		}
	}
}

#define LCD_C 320
#define LCD_R 240
void BmTftAddDisplayFrame(const cv::Mat &frame)
{
	cv::Mat img_resize;
	
	#if 0
	cv::resize(frame, img_resize,  Size(320, 240) );
	#else
	cv::Mat tmp,img_tmp(LCD_R, LCD_C, CV_8UC3,Scalar(0,0,0));
	int x = LCD_C;
	int y = LCD_R;
	int scale_c = (frame.cols/32);
	int scale_r = (frame.rows/24);
	if(scale_c > scale_r)
	{
		//x = LCD_C;
		y = (frame.rows*10/scale_c);
	}
	else
	{
		x = (frame.cols*10/scale_r);
		//y = LCD_R;
	}
	cv::resize(frame, tmp,  Size(x, y) );
	//cout<<"imageResize size : cols = "<<tmp.cols<<", rows = "<<tmp.rows<<endl;
	cv::Mat imageROI = img_tmp(cv::Rect((320 - tmp.cols)/2,(240 - tmp.rows)/2,tmp.cols,tmp.rows));
	tmp.copyTo(imageROI);
	img_resize = img_tmp;
	#endif
	
	
	std::lock_guard<std::mutex> locker(tft_lock_);
	tft_imagebuffer.push(img_resize);
	if (tft_imagebuffer.size() > 3)
	{
		tft_imagebuffer.pop();
		LOG(LOG_DEBUG_NORMAL,cout<<"queue full"<<endl);
	}
	tft_available_.notify_one();
	LOG(LOG_DEBUG_NORMAL,cout<<"queue size : "<<tft_imagebuffer.size()<<endl);
}


int CliCmdEnableTft(int argc, char *argv[])
{
	int level;
	if(argc == 1)
	{
		cout<<"Usage: tft enable"<<endl;
		return 0;
	}
	else //if(argc == 2)
	{
		if((!strcmp(argv[1],"enable")))
		{
			tft_enable = true;
			BmTftInit();
			pthread_create(&tft_tid,NULL,BmTftThread,NULL);
			cout<<"tft_enable : "<<tft_enable<<endl;
			//return 0;
		}
		if((!strcmp(argv[2],"speed")))
		{
			speed = atoi(argv[3]);
		}
		return 0;
	}
	cout<<"Input Error !!\n"<<endl;
	return -1;
}


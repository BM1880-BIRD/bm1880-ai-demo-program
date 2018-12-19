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
static uint32_t mode;
static uint8_t bits = 8;
static char *input_file;
static char *output_file;
static uint32_t speed = 25000000;
static uint16_t delay = 0;
int fd_spidev;
uint8_t tft_display_buf[TFT_COL*TFT_ROW*2];
struct spi_ioc_transfer tr;
bool tft_enable = false;

static void BmSpiTransfer(int fd, uint8_t const *tx, uint8_t const *rx, size_t len)
{
	int ret;
	int out_fd;

	tr.tx_buf = (unsigned long)tx;
	tr.rx_buf = (unsigned long)rx;
	tr.len = len;
	tr.delay_usecs = delay;
	tr.speed_hz = speed;
	tr.bits_per_word = bits;

	if (mode & SPI_TX_QUAD)
		tr.tx_nbits = 4;
	else if (mode & SPI_TX_DUAL)
		tr.tx_nbits = 2;
	if (mode & SPI_RX_QUAD)
		tr.rx_nbits = 4;
	else if (mode & SPI_RX_DUAL)
		tr.rx_nbits = 2;
	if (!(mode & SPI_LOOP)) {
		if (mode & (SPI_TX_QUAD | SPI_TX_DUAL))
			tr.rx_buf = 0;
		else if (mode & (SPI_RX_QUAD | SPI_RX_DUAL))
			tr.tx_buf = 0;
	}

	ret = ioctl(fd, SPI_IOC_MESSAGE(1), &tr);
	if (ret < 1)
	{
		perror("can't send spi message");
	}
}

static void BmSpiSenddata(uint8_t data)
{
	BmSpiTransfer(fd_spidev, &data, NULL, sizeof(data));
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

	BlockWrite(0,TFT_COL-1,0,TFT_ROW-1);

	system("echo 1 > /sys/class/gpio/gpio487/value");//RS=1;

	for(i=0;i<TFT_ROW;i++)
	{
	    for(j=0;j<TFT_COL;j++)
		{    
			bm_tft_SendDataSPI(color>>8);
			bm_tft_SendDataSPI(color);
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

	#ifdef PERF_TEST
	struct timeval start_time,end_time;
	gettimeofday(&start_time,NULL);
	#endif
	
	TftSt7789vBlockWrite(0,TFT_COL-1,0,TFT_ROW-1);
	BmGpioSetValue(TFT_RD_GPIO,1); //RD=1
	p = picture;

	for(i=0; i< (TFT_COL*TFT_ROW*2)/4096; i++)
	{
		BmSpiTransfer(fd_spidev, (p + i*4096), NULL, 4096);	
	}
	BmSpiTransfer(fd_spidev, (p + i*4096), NULL, (320*240*2)%4096);

	#ifdef PERF_TEST
	gettimeofday(&end_time,NULL);
	float cap_cost = (end_time.tv_sec - start_time.tv_sec)*1000000.0 + end_time.tv_usec-start_time.tv_usec;
	cout<<"cap costtime: "<<cap_cost<<" us."<<endl;
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
	LOG(LOG_DEBUG_NORMAL,cout<<"bits per word: "<<bits<<endl);
	LOG(LOG_DEBUG_NORMAL,cout<<"max speed:"<<speed<<"Hz "<<"\"("<<(speed/1000)<<" KHz)\""<<endl);
	
	TftSt7789vInit();
	//DispBand();
	//DispColor(RED);
	BmTftDisplayPicture(logo_hex_8);
	//close(fd_spidev);
	return ret;
}

void BmTftDisplayFrame(const cv::Mat &frame)
{
	cv::Mat srcImage;
	cv::resize(frame, srcImage,  Size(320, 240) );
	//cv::imwrite("/mnt/usb/wl_test1.jpg",frame);
	//cv::imwrite("/mnt/usb/wl_test2.jpg",srcImage);
	//imshow("\u663e\u793a\u56fe\u50cf", srcImage);  

	int rowNumber = srcImage.rows;  
	int colNumber = srcImage.cols;  
	int i,j;
	for (i = 0; i < rowNumber; i++)
	{
		for (j= 0; j < colNumber; j++)
		{
			unsigned short B = srcImage.at<Vec3b>(i, j)[0];	//B
			unsigned short G = srcImage.at<Vec3b>(i, j)[1];	//G
			unsigned short R = srcImage.at<Vec3b>(i, j)[2];	//R
			//hex 8
			unsigned short pix_data = ((R << 8) & 0xF800) | ((G << 3) & 0x07E0) | ((B >> 3) & 0x001F);
			tft_display_buf[i*colNumber*2 + j*2 ] = (char)((pix_data >> 8) & 0xFF); 
			tft_display_buf[i*colNumber*2 + j*2 + 1] = (char)(pix_data & 0xFF); 
		}
	}
	BmTftDisplayPicture(tft_display_buf);
}

int CliCmdEnableTft(int argc, char *argv[])
{
	int level;
	if(argc == 1)
	{
		cout<<"Usage: tft enable"<<endl;
		return 0;
	}
	else if(argc == 2)
	{
		if((!strcmp(argv[1],"enable")))
		{
			tft_enable = true;
			BmTftInit();
			cout<<"tft_enable : "<<tft_enable<<endl;
			return 0;
		}
	}
	cout<<"Input Error !!\n"<<endl;
	return -1;
}


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

#ifndef _BM_SPI_TFT_H_
#define _BM_SPI_TFT_H_
#include <queue>
#include <mutex>
#include <thread>
#include <condition_variable>

#define TFT_COL  320
#define TFT_ROW  240

#define RED    0xF800
#define GREEN  0x07E0
#define BLUE   0x001F
#define WHITE  0xFFFF
#define BLACK  0x0000
#define GRAY75 0x39E7 
#define GRAY50 0x7BEF	
#define GRAY25 0xADB5

#define TFT_SPI_DEV "/dev/spidev32766.0"

//GPIO7 
#define TFT_RD_GPIO (487) 
//GPIO51
#define TFT_RESET_GPIO (467) 

extern uint8_t tft_display_buf[];
extern bool tft_enable;


extern std::queue<cv::Mat>tft_imagebuffer;
extern std::condition_variable tft_available_;
extern std::mutex tft_lock_;


int BmTftInit(void);
void BmTftDisplayFrame(const cv::Mat &frame);
void BmTftAddDisplayFrame(const cv::Mat &frame);
int CliCmdEnableTft(int argc, char *argv[]);

#endif

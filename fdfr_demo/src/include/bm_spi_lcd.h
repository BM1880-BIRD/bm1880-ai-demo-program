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

//#define CUSTOM_ZKT

#define LCD_COL  320
#define LCD_ROW  240

#define RED    0xF800
#define GREEN  0x07E0
#define BLUE   0x001F
#define WHITE  0xFFFF
#define BLACK  0x0000
#define GRAY75 0x39E7
#define GRAY50 0x7BEF
#define GRAY25 0xADB5

#define LCD_SPI_DEV "/dev/spidev32766.0"
#define EDB_GPIO(x) (x<32)?(480+x):((x<64)?(448+x-32):((x<=67)?(444+x-64):(-1)))

//D/CK
#define LCD_RD_GPIO EDB_GPIO(7)
//LCD_RESET
#ifdef CUSTOM_ZKT
#define LCD_RESET_GPIO EDB_GPIO(9)
#define LCD_EN_GPIO EDB_GPIO(10)

#else
#define LCD_RESET_GPIO EDB_GPIO(51)
#endif

extern uint8_t lcd_display_buf[];

extern std::queue<cv::Mat>lcd_imagebuffer;
extern std::condition_variable lcd_available_;
extern std::mutex lcd_lock_;


int BmLcdInit(void);
void BmLcdDisplayFrame(const cv::Mat &frame);
void BmLcdAddDisplayFrame(const cv::Mat &frame);
int CliCmdEnableLcd(int argc, char *argv[]);

#endif

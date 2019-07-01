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

#define LCD_COL  320
#define LCD_ROW  240

#define RED    0xF800
#define GREEN  0x07E0
#define BLUE   0x001F
#define WHITE  0xFFFF
#define BLACK  0x0000
#define GRAY   0xEF5D
#define GRAY75 0x39E7
#define GRAY50 0x7BEF
#define GRAY25 0xADB5

#define LCD_SPI_DEV "/dev/spidev32766.0"
#define EDB_GPIO(x) (x<32)?(480+x):((x<64)?(448+x-32):((x<=67)?(444+x-64):(-1)))

//GPIO7
#define LCD_RD_GPIO EDB_GPIO(7)
//GPIO51
#define LCD_RESET_GPIO EDB_GPIO(51)
//#define LCD_RESET_GPIO EDB_GPIO(9)


#define log_level 4

#define LOG_DISABLE (0)
#define LOG_DEBUG_ERROR (1)
#define LOG_DEBUG_NORMAL (2)
#define LOG_DEBUG_USER_3 (3)
#define LOG_DEBUG_USER_4 (4)
#define LOG_DEBUG_USER_(x) (x)

#define  BM_LOG(x,fmt...) \
        do{\
                if(x <= log_level)\
                {\
                        fmt;\
                }\
        }while(0)

#endif

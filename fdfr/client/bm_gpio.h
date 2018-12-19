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

#ifndef _BM_GPIO_H_
#define _BM_GPIO_H_

int BmGpioSetValue(unsigned int gpio, unsigned int value);
int BmGpioGetValue(unsigned int gpio, unsigned int *value);
int BmGpioUnexport(unsigned int gpio);

#endif

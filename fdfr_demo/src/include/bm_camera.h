/*
 ********************************************************************
 * Demo program on Bitmain BM1880
 *
 * Copyright © 2018 Bitmain Technologies. All Rights Reserved.
 *
 * Author:  liang.wang02@bitmain.com
 *			xiaojun.zhu@bitmain.com
 ********************************************************************
*/
#ifndef __BM_CAMERA_H__
#define __BM_CAMERA_H__

#define UVC_PR_X  (1280)
#define UVC_PR_Y  (720)

#define SUPPORT_ZKT_FAM600_CAMERA


int BmCameraGetFrame(cv::Mat &matFrame);
int BmFaceDisplay(cv::Mat &frame);

extern cv::Mat bm_testframe;
#endif


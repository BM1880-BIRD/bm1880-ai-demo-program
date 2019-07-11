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
#ifndef __BM_SOCKET__
extern std::queue<cv::Mat>imagebuffer;
extern std::condition_variable available_;
extern std::mutex lock_;
extern pthread_t sk_tid;

void BmFaceSendSocket(cv::Mat frame);
void* BmFaceSocketThread(void *arg);

#endif

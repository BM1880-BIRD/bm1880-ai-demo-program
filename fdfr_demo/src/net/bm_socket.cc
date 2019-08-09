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
#include <iostream>
#include <string>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <errno.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <queue>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <opencv2/opencv.hpp>

using namespace std;
using namespace cv;

#include "bm_common_config.h"
#include "bm_socket.h"


std::queue<cv::Mat>imagebuffer;
std::condition_variable available_;
std::mutex lock_;
pthread_t sk_tid;

struct sockaddr_in servaddr;


void BmFaceSendSocket(cv::Mat frame)
{
	int sockfd;
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		exit(0);
	}

	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = PF_INET;
	servaddr.sin_port = htons(bm1880_config.host_port);
	if (inet_pton(AF_INET, bm1880_config.host_ip_addr.c_str(), &servaddr.sin_addr) <= 0) {
		//LOG(LOG_DEBUG_ERROR,cout<<"inet_pton error"<<endl);
		exit(0);
	}

	if (connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0) {
		//LOG(LOG_DEBUG_ERROR,cout<<"connect error: "<<(strerror(errno))<<"errno: "<<(errno)<<endl);
		exit(0);
	}

	//LOG(LOG_DEBUG_NORMAL,cout<<"ready to send buffer to server: "<<endl);

	std::vector<uchar> data_encode;
	cv::imencode(".jpg", frame, data_encode);

#if 0
	for(int i = 0 ;i <frame->rows;i++)
	{
		uchar *data = frame->ptr<uchar>(i);
		int num = i*frame->cols*3;
		for(int j = 0; j<frame->cols*3; j++)
		{
			sendData[num+j]=(char)data[j];
		}
	}
	LOG(LOG_DEBUG_USER_4,cout<<"senddata size: "<<(sizeof(sendData))<<endl);
#endif

	FILE *fp = fdopen(sockfd, "w");
	if (fp == nullptr) {
		fprintf(stderr, "failed to fdopen\n");
		::close(sockfd);
	}
	//printf("send data size:%d\n", data_encode.size());
	string mimetype = "image/jpeg";
	string url = "/display";
	fprintf(fp, "POST %s HTTP/1.1\n", url.data());
	fprintf(fp, "Content-Type: %s\n", mimetype.data());
	fprintf(fp, "Content-Length: %lu\n", data_encode.size());
	fprintf(fp, "\n");
	fwrite(data_encode.data(), sizeof(unsigned char), data_encode.size(), fp);
	fclose(fp);
#if 0
	int bytes=send(sockfd,sendData,sizeof(sendData),0);

	if(bytes<0)
	{
		LOG(LOG_DEBUG_ERROR,cout<<"send msg error:"<<(strerror(errno))<<"errno "<<(errno)<<endl);
		exit(0);
	}

	LOG(LOG_DEBUG_USER_4,cout<<"send bytes: "<<(bytes)<<endl);

	close(sockfd);
#endif
}


void* BmFaceSocketThread(void *arg)
{
	//static int i = 0;
	while (true) {
		std::unique_lock<std::mutex> locker(lock_);
		while(imagebuffer.empty())
			available_.wait(locker);

			auto image = std::move(imagebuffer.front());
			imagebuffer.pop();
			locker.unlock();
			//LOG(LOG_DEBUG_USER_4,"start to send\n");
			BmFaceSendSocket(image);
			//printf("BmFaceSocketThread : %d .\n", i++);
			//LOG(LOG_DEBUG_USER_4,"send done\n");
			//usleep(50000);

	}
}



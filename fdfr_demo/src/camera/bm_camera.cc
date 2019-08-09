#include <iostream>
#include <string>
#include <cstdio>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <cstdlib>
#include <opencv2/videoio.hpp>
#include <opencv2/videoio/videoio_c.h>
#include <opencv2/core/core.hpp>
#include <opencv2/opencv.hpp>

using namespace std;

#include "bm_common_config.h"
#include "bm_camera.h"

cv::VideoCapture UvcCapture;
bool BmUvcCameraInit(void)
{
	static bool HasInit = false;
	if (HasInit == true)
		return true;
	//capture frame from  uvc camera
	UvcCapture.open(200 + bm1880_config.camera_id);

	if (UvcCapture.isOpened()) {
		BM_LOG(LOG_DEBUG_USER_(3),cout<<"open successful ."<<endl);
	} else {
		BM_LOG(LOG_DEBUG_ERROR,cout<<"open fail !!"<<endl);
		return false;
	}

	UvcCapture.set(CV_CAP_PROP_FOURCC,CV_FOURCC('N','V','1','2'));
	UvcCapture.set(CV_CAP_PROP_FRAME_WIDTH, bm1880_config.camera_width);
	UvcCapture.set(CV_CAP_PROP_FRAME_HEIGHT, bm1880_config.camera_height);
	UvcCapture.set(CV_CAP_PROP_FPS, 15);

	HasInit = true;
	return true;
}

int BmUvcGetFrame(cv::Mat &matFrame)
{
	cv::Mat mat;
	if(BmUvcCameraInit() == false)
		return -1;

	UvcCapture >> matFrame;
	//cv::flip(mat, matFrame, 1);

	return 0;
}

int BmCameraGetFrame(cv::Mat &MatFrame)
{
	if (bm1880_config.frame_source.empty())	{
		BM_LOG(LOG_DEBUG_ERROR, cout<<"Please set frame_source [uvc/rtsp/pic/dir].");
		return -1;
	}

	if (bm1880_config.frame_source == "uvc") {
		BmUvcGetFrame(MatFrame);
	} else if(bm1880_config.frame_source == "rtsp") {
		;
	}
	else if (bm1880_config.frame_source == "pic") {
		static bool __read = false;
		if (__read == false) {
			cout << "1.read pic: " << bm1880_config.load_pic_path << endl;
			MatFrame = cv::imread(bm1880_config.load_pic_path);
			__read = true;
		}
		if (MatFrame.empty())
			__read = false;
	} else if (bm1880_config.frame_source == "dir"){
		static bool __init = false;
		static vector<string> all_files;
		static int frame_num = 0;
		if (__init == false) {
			BmListAllFiles(bm1880_config.load_pic_path, all_files);
			__init = true;
		}
		if (frame_num < all_files.size()) {
			cout << "2.read pic: " << all_files[frame_num] << endl;
			MatFrame = cv::imread(all_files[frame_num++]);
		} else {
			//all_files.clear();
			cout << "load images finish." << endl;
			vector<string> files;
			all_files.swap(files);
			frame_num = 0;
			__init = false;
		}
	} else if (bm1880_config.frame_source == "dir_frame"){
		static int frame_num = 0;
		string file_name = bm1880_config.load_pic_path + to_string(frame_num) + ".png";
		cout << "Read " << file_name << endl;
		MatFrame = cv::imread(file_name);
		frame_num ++;
	}

	if (MatFrame.empty()) {
		BM_LOG(LOG_DEBUG_ERROR, cout<<"frame empty"<<endl);
		return -3;
	}

	return 0;
}

cv::Mat bm_testframe;
int BmCliCmdGetFrame(int argc,char *argv[])
{
	int ret = 0;
	if (argc == 1) {
		ret = BmCameraGetFrame(bm_testframe);
		if (ret == 0) {
			BmFaceDisplay(bm_testframe);

			cout << "Get one frame." << endl;
		}
	} else if(argc == 2) {
		if (!strcmp(argv[1],"save")) {
			string file_name = bm1880_config.record_path+"/"+GetSystemTime()+".png";
			cv::imwrite(file_name, bm_testframe);
			if (!access(file_name.c_str(), 0)) {
				cout << file_name << " save ok." << endl;
			} else {
				cout << file_name << " save fail." << endl;
			}

		}
	}
	return ret;
}


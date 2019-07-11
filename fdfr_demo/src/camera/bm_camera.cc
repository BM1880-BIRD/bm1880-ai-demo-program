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
using namespace cv;

#include "bm_common_config.h"
#include "bm_camera.h"

int u4PicRdValue = 0;


cv::VideoCapture UvcCapture;
bool BmUvcCameraInit(void)
{
	static bool HasInit = false;
	if (HasInit == true)
		return true;
	//capture frame from  uvc camera
	UvcCapture.open(200);

	if (UvcCapture.isOpened()) {
		BM_LOG(LOG_DEBUG_USER_(3),cout<<"open successful ."<<endl);
	} else {
		BM_LOG(LOG_DEBUG_ERROR,cout<<"open fail !!"<<endl);
		return false;
	}

	UvcCapture.set(CV_CAP_PROP_FOURCC,CV_FOURCC('N','V','1','2'));
	UvcCapture.set(CV_CAP_PROP_FRAME_WIDTH,UVC_PR_X);
	UvcCapture.set(CV_CAP_PROP_FRAME_HEIGHT,UVC_PR_Y);
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
	if (edb_config.source_type == "pic") {
		static bool __read = false;
		if (__read == false) {
			cout << "1.read pic: " << edb_config.load_pic_path << endl;
			MatFrame = cv::imread(edb_config.load_pic_path);
			__read = true;
		} else 
			return -3;//empty
	} else if (edb_config.source_type == "dir"){
		static bool __init = false;
		static vector<string> all_files;
		static int frame_num = 0;
		if (__init == false) {
			BmListAllFiles(edb_config.load_pic_path, all_files);
			__init = true;
		}
		if (frame_num < all_files.size()) {
			cout << "2.read pic: " << all_files[frame_num] << endl;
			MatFrame = cv::imread(all_files[frame_num++]);
			if (edb_config.step == true) {
				char c = getchar();
			}
		} else {
			all_files.clear();
			return -3;//empty
		}
	} else if (edb_config.source_type == "dir_frame"){
		static int frame_num = 0;
		string file_name = edb_config.load_pic_path + to_string(frame_num) + ".png";
		cout << "Read " << file_name << endl;
		MatFrame = cv::imread(file_name);
		frame_num ++;
		if (edb_config.step == true) {
			char c = getchar();
		}
	} else if (edb_config.source_type == "cam") {
		if (edb_config.camera_type.empty())	{
			BM_LOG(LOG_DEBUG_ERROR, cout<<"Please set camera type: uvc/fam600.");
			return -1;
		}
		if (edb_config.camera_type == "uvc") {
			BmUvcGetFrame(MatFrame);
		}
	}
	if (MatFrame.empty()) {
		BM_LOG(LOG_DEBUG_ERROR, cout<<"frame empty"<<endl);
		return -3;
	}

	return 0;
}





int BmCliCmdGetFrame(int argc,char *argv[])
{
	cv:Mat testFrame, imageResize;
	//cout<<__FUNCTION__<<"  "<<__LINE__<<endl;
	//system("rm /fam600/record/test.jpg");
	//Fam600GetFrame(testFrame);
	//testFrame = cv::imread("/fam600/1180637_2.jpg");
	//cout<<"frame_src width: "<<testFrame.cols<<" height: "<<testFrame.rows<<" ."<<endl;

	//int x = round(testFrame.cols/4) ;
	//int y = round(testFrame.rows/4);
	//cout<<"scale: "<<4<<", x= "<<x<<", y= "<<y<<" ."<<endl;
	//cv::resize(testFrame, imageResize,  Size(x, y) );
	//cout<<"frame_src width: "<<imageResize.cols<<" height: "<<imageResize.rows<<" ."<<endl;

	//while(1)
	{
		BmCameraGetFrame(testFrame);
		BmFaceDisplay(testFrame);


		//BM_LOG(LOG_DEBUG_NORMAL, cout<<"save "+frame_record + "/" + to_string(u4PicRdValue) + ".jpg");
		//cv::imwrite(frame_record + "/" + to_string(u4PicRdValue) + ".jpg",testFrame);
		//u4PicRdValue++;
	}
	//cv::imwrite("/fam600/test_test.jpg",testFrame);
	//cv::imwrite("/fam600/test_resize.jpg",imageResize);


	cout<<"test done"<<endl;
}


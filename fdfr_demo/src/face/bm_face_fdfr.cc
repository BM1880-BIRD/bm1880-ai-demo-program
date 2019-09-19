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
#include <opencv2/videoio.hpp>
#include <opencv2/videoio/videoio_c.h>
#include <opencv2/core/core.hpp>
#include <opencv2/opencv.hpp>
#include <queue>
#include <mutex>
#include <thread>
#include <condition_variable>

#include "fdfr.hpp"
#include "face_impl.hpp"

#include "image.hpp"
#include "face_api.hpp"
#include "tuple.hpp"
#include "file_utils.hpp"
#include "qnn.hpp"

using namespace std;
using namespace cv;
using namespace qnn::vision;
using namespace qnn::utils;

#include "bm_common_config.h"
#include "bm_face.h"
#include "bm_spi_lcd.h"
#include "bm_socket.h"
#include "bm_camera.h"


int bmface_log_level = LOG_DEBUG_NORMAL;

//face_algorithm_t fd_algo = FD_TINYSSH;

//
pthread_t bmface_tid;

//queue &signal for lcd display
typedef struct __bmpicrd {
	cv::Mat image;
	string name;
	string txtinfo;
} BMPICRD_T;
std::deque<BMPICRD_T> bmpicrd_imagebuffer;
std::condition_variable bmpicrd_available_;
std::mutex bmpicrd_lock_;
//std::deque<BMPICRD_T> q;
//std::mutex mu;
//std::condition_variable cond;

pthread_t bmpic_record_tid;


//threshold
double sim_threshold = 0.6;

string person_name ="";

vector<vector<string>> detector_models = {
	{//FD_TINYSSH
		"/system/data/bmodel/Detection/Tiny_SSH/320x320/tiny_ssh.bmodel",
		"/system/data/bmodel/Detection/Tiny_SSH/320x320/det3.bmodel",
	},
	{//FD_MTCNN
		"",
	},
	{//FD_MTCNN_NATIVE
		"/system/data/bmodel/Detection/MTCNN/nchw/det1.bmodel",
		"/system/data/bmodel/Detection/MTCNN/nchw/det2.bmodel",
		"/system/data/bmodel/Detection/MTCNN/nchw/det3.bmodel",
	},
	{//FD_SSH
		"",
	},
	{//Face spoof
		"/system/data/bmodel/anti-facespoofing/face_spoof_0703.bmodel",
		"/system/data/bmodel/anti-facespoofing/face_spoof_0702.bmodel",
		"/system/data/bmodel/anti-facespoofing/face_spoof_0706.bmodel",
	}
};

vector<vector<string>> extractor_models =
{
	{
		"/system/data/bmodel/bmface.bmodel",
	},
};

int BmFaceDisplayInit(void)
{
	static bool hasinit = false;
	if (hasinit == false) {
		if (bm1880_config.host_ip_addr.empty() && (bm1880_config.lcd_display == false)) {
			BM_FACE_LOG(LOG_DEBUG_ERROR,
				cout<<" Please set host ip address or enable tft lcd first."<<endl);
			return -1;
		}

		if (!bm1880_config.host_ip_addr.empty()) {
			pthread_create(&sk_tid,NULL,BmFaceSocketThread,NULL);
		}

		if (bm1880_config.lcd_display) {
			BmLcdInit();
		}

		hasinit = true;
	}
	return 0;
}

int BmFaceDisplay(cv::Mat &frame)
{
	BmFaceDisplayInit();

	//send data to lcd.
	if (bm1880_config.lcd_display) {
		BmLcdAddDisplayFrame(frame);
	}

	//send data to socket.
	if (!bm1880_config.host_ip_addr.empty()) {
		cv::Mat img = frame.clone();
		std::lock_guard<std::mutex> locker(lock_);

		if (imagebuffer.size() <= 3) {
			imagebuffer.push(img);
		}
		available_.notify_one();
		BM_FACE_LOG(LOG_DEBUG_USER_(3), cout << "queue size : " << imagebuffer.size() << endl);
	}
}

unique_ptr<FDFR> bm_fdfr = nullptr;
int BmFaceSetupFdFrModel(void)
{
	if (bm_fdfr == nullptr) {//create FDFR & FaceImpl instance
		face_algorithm_t fd_algo = (face_algorithm_t)bm1880_config.fd_algo;
		printf("%s: %d .\n",__FUNCTION__, fd_algo);
		bm_fdfr.reset(new FDFR(fd_algo, detector_models[fd_algo], FR_BMFACE, extractor_models[0]));
	}
	BM_FACE_LOG(LOG_DEBUG_NORMAL, cout << "Repo size: " << bm_repo.id_list().size() << endl);
	if (bm_repo.id_list().size() == 0) {
		BM_FACE_LOG(LOG_DEBUG_ERROR, cout << "Repo is NULL, Please check !!" << endl);
		bm1880_config.do_fr = 0;
	}
	return 0;
}
void* BmFaceThread(void *arg)
{
	int Ret = 0;

	struct timeval start_time,end_time;
	string s_time = GetSystemTime();
	int frame_num = 0;

	if(BmFaceSetupFdFrModel() != 0) {
		pthread_exit(NULL);
	}
	//if(BmFaceSetupAfsClassify() != 0) {
	//	pthread_exit(NULL);
	//}
	//while loop for bmface thread
	while (1) {
		cv::Mat frame;
		vector<face_info_t> results;
		vector<face_info_t> face_infos;
		vector<face_features_t> features_list;
		bool liveness = false;



		if (bm1880_config.pause == true) {
			char c = getchar();
			printf("getchar 0x%x .\n", c);
			bm1880_config.pause = false;
		}
		if (bm1880_config.stop == true) {
			bm1880_config.stop = false;
			break;
		}
		if (bm1880_config.step == true) {
			char c = getchar();
			if ( c == 'e')
			{
				cout << "step mode : false ." << endl;
				bm1880_config.step = false;
			}
		}


		if (!BmCameraGetFrame(frame)) {
			BM_FACE_LOG(LOG_DEBUG_USER_(3),
				cout<<"frame width: "<<frame.cols<<", height: "<<frame.rows<<endl);
		}
		else
		{
			pthread_exit(NULL);
		}
		bm_testframe = frame.clone();

		bm_fdfr->detect(frame, results);
		bm_fdfr->fd_time.print();


		//=========================================================================================================
		BM_FACE_LOG(LOG_DEBUG_NORMAL, cout << "Total " << results.size() << " faces detected."<<endl);
		if (results.size() == 0)
			continue;
		std::sort(results.begin(), results.end(), [](face_info_t &a, face_info_t &b) {
				return (a.bbox.x2 - a.bbox.x1 + 1) * (a.bbox.y2 - a.bbox.y1 + 1)
						> (b.bbox.x2 - b.bbox.x1 + 1) * (b.bbox.y2 - b.bbox.y1 + 1);
		});

		if (bm1880_config.fd_only_maximum == true) {
			int face_size = (int)((results[0].bbox.x2 - results[0].bbox.x1 + 1) * (results[0].bbox.y2 - results[0].bbox.y1 + 1));
			if (face_size < 3000)
				continue;
			else
				face_infos.emplace_back(results[0]);
		} else {
			face_infos.swap(results);
		}

		if (bm1880_config.afs_enable)
		{
			//liveness = BmFaceRunAfs(frame, face_infos[0]);
			//BM_FACE_LOG(LOG_DEBUG_USER_3, cout << "" << (liveness?"Real":"Fake") << endl);
		}

		//printf("do_fr = %d .\n", bm1880_config.do_fr);
		if (bm1880_config.do_fr > 0) {
			cv:Mat frame_img = frame.clone();

			bm_fdfr->extract(frame,face_infos,features_list);
			bm_fdfr->align_time.print();
			bm_fdfr->fr_time.print();

			for (int i = 0; i < face_infos.size(); i++) {
				auto match_result = bm_repo.match(features_list[i].face_feature.features, sim_threshold);

				BM_FACE_LOG(LOG_DEBUG_USER_3,cout << "match_result: " << match_result.matched
					<< " " << match_result.id << " " << match_result.score << endl);

				string match_name = "nuknown";
				if(match_result.matched == true) {
					match_name = bm_repo.get_name(match_result.id).value_or("");
				}

				float x1 = face_infos[i].bbox.x1;
				float y1 = face_infos[i].bbox.y1;
				float x2 = face_infos[i].bbox.x2;
				float y2 = face_infos[i].bbox.y2;

				if (liveness && (i == 0)) {
					cv::rectangle(frame, cv::Point(x1, y1),	cv::Point(x2, y2), cv::Scalar(0, 255, 0), 3, 8, 0);
				} else {
					cv::rectangle(frame, cv::Point(x1, y1),	cv::Point(x2, y2), cv::Scalar(0, 0, 255), 3, 8, 0);
				}

				int pos_x = std::max(int(x1 - 5), 0);
				int pos_y = std::max(int(y1 - 10), 0);
				cv::putText(frame, match_name, cv::Point(pos_x, pos_y), cv::FONT_HERSHEY_PLAIN, 4.0, CV_RGB(0,255,255), 2.0);

				#ifdef BM_FDFR_DEBUG
				if (bm1880_config.record_image == true) {
					stringstream strStream << "echo \"" << bm1880_config.do_fr << ": "
						<< ((match_name == person_name)?"Fail":"Pass")\
						<< "| name: " << match_name \
						<< "| score: " << match_result.score\
						<< "\" >> " << (bm1880_config.record_path+"/"+person_name)+"/"+person_name+".txt";
					cv::Mat test;
					//cout << "txt: " << strStream.str() << endl;
					BmImageRecord(test, "", strStream.str());
				}
				#endif
			}
			#ifdef BM_FDFR_DEBUG
			if (bm1880_config.record_image == true) {
				string pic_name = bm1880_config.record_path+"/" + person_name+"/" + person_name + "_" + to_string(bm1880_config.do_fr)+"_ori.png";
				BmImageRecord(frame_img, pic_name, "");
				pic_name = bm1880_config.record_path+"/" + person_name+"/" + person_name + "_" + to_string(bm1880_config.do_fr)+".png";
				BmImageRecord(frame, pic_name, "");
			}
			#endif
			if (bm1880_config.do_fr < 10000)
				bm1880_config.do_fr --;
		} else {
			if ((bm1880_config.camera_capture_mode) && (bm1880_config.record_image == true)) {
				string pic_name = bm1880_config.record_path + "/" +
					s_time + "_frame_"+ to_string(frame_num++) + ".png";

				cout << "name : " << pic_name << endl;
				//cv::imwrite(_name, frame);
				BmImageRecord(frame, pic_name, "");
			}

			for (int i = 0; i < face_infos.size(); i++) {
				if (liveness && (i == 0)) {
					cv::rectangle(frame, cv::Point(face_infos[i].bbox.x1, face_infos[i].bbox.y1),
						cv::Point(face_infos[i].bbox.x2, face_infos[i].bbox.y2), cv::Scalar(0, 255, 0), 3, 8, 0);
				} else {
					cout << "mark 1 " << endl;
					cv::rectangle(frame, cv::Point(face_infos[i].bbox.x1, face_infos[i].bbox.y1),
						cv::Point(face_infos[i].bbox.x2, face_infos[i].bbox.y2), cv::Scalar(0, 0, 255), 3, 8, 0);
				}

				#if 0
				face_pts_t facePts = face_infos[i].face_pts;
				for (int j = 0; j < 5; j++) {
					cv::circle(frame, cv::Point(facePts.x[j], facePts.y[j]), 1, cv::Scalar(255, 255, 0), 8);
				}
				#endif
			}
		}

		PERFORMACE_TIME_MESSURE_START(start_time);
		BmFaceDisplay(frame);
		PERFORMACE_TIME_MESSURE_END(end_time);
	}

	bmface_tid = 0;
	pthread_exit(NULL);
}

int BmImageRecord(const cv::Mat &frame, const string &name, const string &info)
{
	BMPICRD_T rd;

	if (!frame.empty())
		rd.image = frame.clone();
	if (!name.empty()){
		//cout << "name: " << name << endl;
		rd.name = name;
	}
	if (!info.empty())
		rd.txtinfo = info;
	std::unique_lock<std::mutex> locker(bmpicrd_lock_);
    bmpicrd_imagebuffer.push_front(rd);
    locker.unlock();
    bmpicrd_available_.notify_one();  // Notify one waiting thread, if there is one.
	//cout << " notify_one !!" << endl;   
}



void* BmPicRecordThread(void *arg)
{
    while (1) {
        std::unique_lock<std::mutex> locker(bmpicrd_lock_);
        while(bmpicrd_imagebuffer.empty())
            bmpicrd_available_.wait(locker); // Unlock mu and wait to be notified
        auto tmp = bmpicrd_imagebuffer.back();
        bmpicrd_imagebuffer.pop_back();
        locker.unlock();
		if (!tmp.name.empty()) {
			BM_LOG(LOG_DEBUG_USER_(2),cout<<"start record frame: " << tmp.name << endl);
			cv::imwrite(tmp.name, tmp.image);
		}
		if (!tmp.txtinfo.empty())
			system(tmp.txtinfo.c_str());

		system("sync");
		//std::this_thread::sleep_for(std::chrono::seconds(1));
        //std::cout << "t2 got a value from t1: " << data << std::endl;
    }
}


int CliCmdfdfr(int argc,char *argv[])
{
	if (argc == 1) {
		cout<<"Usage: fdfr [start|pause|stop]"<<endl;
		return 0;
	} else if(argc == 2) {
		if (!strcmp(argv[1],"start")) {

			pthread_create(&bmface_tid, NULL, BmFaceThread,NULL);
			if ((bm1880_config.record_image == true) || (bm1880_config.afs_debug_frame_record_real == true)
					|| (bm1880_config.afs_debug_frame_record_fake == true)) {
				pthread_create(&bmpic_record_tid, NULL, BmPicRecordThread,NULL);
			}
			//cout<<"Open uvc camera ."<<endl;
			return 0;
		} else if(!strcmp(argv[1],"pause")) {
			bm1880_config.pause = true;
			cout<<"Pause bmface thread ."<<endl;
			return 0;
		} else if(!strcmp(argv[1],"stop")) {
			bm1880_config.stop = true;
			cout<<"Stop bmface thread ."<<endl;
			return 0;
		} else if(!strcmp(argv[1],"step")) {
			bm1880_config.step = true;
			cout<<"Set step mode : true."<<endl;
			return 0;
		}
	}

	cout<<"Input Error !!"<<endl;
	return -1;
}

int CliCmdTestFr(int argc,char *argv[])
{
	char _cmd[200];
	int value;

	if (argc == 1) {
		cout<<"Usage: testfr [num] [name]"<<endl;
		return 0;
	} else if(argc == 2) {
		if (!strcmp(argv[1],"stop")) {
			bm1880_config.do_fr = 0;
		}
	} else if(argc == 3) {
		value = atoi(argv[1]);
		person_name = argv[2];

		sprintf(_cmd,"mkdir -p %s",(bm1880_config.record_path+"/"+person_name).c_str());
		cout<<"system cmd :"<<_cmd<<endl;
		system(_cmd);
		sprintf(_cmd,"rm %s/*",(bm1880_config.record_path+"/"+person_name).c_str());
		cout<<"system cmd :"<<_cmd<<endl;
		system(_cmd);
		sprintf(_cmd,"touch %s/%s.txt",(bm1880_config.record_path+"/"+person_name).c_str(),person_name.c_str());
		cout<<"system cmd :"<<_cmd<<endl;
		system(_cmd);

		bm1880_config.do_fr = value;
	}

	cout<<" face detect is :"<<(bm1880_config.do_fr?"Enable":"Disable")<<" ."<<endl;
	return 0;
}



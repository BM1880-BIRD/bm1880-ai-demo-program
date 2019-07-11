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

face_algorithm_t fd_algo = FD_TINYSSH;
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

pthread_t bmpicrd_tid;


//threshold
double sim_threshold = 0.6;

string face_reg_name;
int do_face_reg = 0;

string person_name ="";

vector<vector<string>> detector_models = {
	{//FD_TINYSSH
		"/system/data/bmodel/Face/Detection/Tiny_SSH/320x320/tiny_ssh.bmodel",
		"/system/data/bmodel/Face/Detection/Tiny_SSH/320x320/det3.bmodel",
	},
	{//FD_MTCNN
		"",
	},
	{//FD_MTCNN_NATIVE
		"/system/data/bmodel/mtcnn/det1.bmodel",
		"/system/data/bmodel/mtcnn/det2.bmodel",
		"/system/data/bmodel/mtcnn/det3.bmodel",
	},
	{//FD_SSH
		"",
	},
	{//Face spoof
		"",
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
		if (edb_config.host_ip_addr.empty() && (edb_config.lcd_display == false)) {
			BM_FACE_LOG(LOG_DEBUG_ERROR,
				cout<<" Please set host ip address or enable tft lcd first."<<endl);
			return -1;
		}

		if (!edb_config.host_ip_addr.empty()) {
			pthread_create(&sk_tid,NULL,BmFaceSocketThread,NULL);
		}

		if (edb_config.lcd_display) {
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
	if (edb_config.lcd_display) {
		BmLcdAddDisplayFrame(frame);
	}

	//send data to socket.
	if (!edb_config.host_ip_addr.empty()) {
		cv::Mat img = frame.clone();
		std::lock_guard<std::mutex> locker(lock_);
		imagebuffer.push(img);
		if (imagebuffer.size() > 3) {
			imagebuffer.pop();
		}
		available_.notify_one();
		BM_FACE_LOG(LOG_DEBUG_USER_(3), cout << "queue size : " << imagebuffer.size() << endl);
	}
}

void* BmFaceThread(void *arg)
{
	int Ret = 0;
	stringstream strStream;
	struct timeval start_time,end_time;
	string s_time = GetSystemTime();
	int fram_num = 0;
	//create FDFR & FaceImpl instance
	FDFR fdfr(fd_algo, detector_models[fd_algo], FR_BMFACE, extractor_models[0]);

	std::unique_ptr<AntiFaceSpoofing> spoof_detector;

	BM_FACE_LOG(LOG_DEBUG_NORMAL,
		cout << "Repo size: " << repo.id_list().size() << endl);

	//while loop for bmface thread
	while (1) {
		cv::Mat frame;
		vector<fd_result_t> results;
		vector<face_info_t> face_infos;
		vector<face_features_t> features_list;
		vector<fr_result_t> face_result;

		if (!BmCameraGetFrame(frame)) {
			BM_FACE_LOG(LOG_DEBUG_USER_(3),
				cout<<"frame width: "<<frame.cols<<", height: "<<frame.rows<<endl);
		}
		else
		{
			pthread_exit(NULL);
		}

		PERFORMACE_TIME_MESSURE_START(start_time);
		fdfr.detect(frame,results);
		PERFORMACE_TIME_MESSURE_END(end_time);
		BM_FACE_LOG(LOG_DEBUG_USER_(3), cout<< "detect done ." << endl);
		//=========================================================================================================
		if (results.size() > 0) {
			float face_size =0, max_size =0;
			int max_size_id = 0;
			for (size_t i = 0; i < results.size(); i++ ) {

				float X1 = results[i].bbox.x1;
				float Y1 = results[i].bbox.y1;
				float X2 = results[i].bbox.x2;
				float Y2 = results[i].bbox.y2;
				BM_FACE_LOG(LOG_DEBUG_USER_3, strStream << "     face " << i+1<< ": ("
								<< results[i].bbox.x1 << ", " << results[i].bbox.y1 << "), ("
								<< results[i].bbox.x2 << ", " << results[i].bbox.y2 << ")"
								<< results[i].bbox.score << endl);

				face_size = (X2-X1)*(Y2-Y1);
				if (i == 0) {
					max_size = face_size;
					max_size_id = i;
				} else {
					if (face_size > max_size) {
						max_size = face_size;
						max_size_id = i;
					}
				}
			}
			BM_FACE_LOG(LOG_DEBUG_USER_3, strStream << "    Total " << results.size() << " faces detected.");
			//cout<<" max_size="<<max_size<<",max_size_id="<<max_size_id<<endl;
			strStream.str("");
			
			if (max_size > 3000) {
				if (edb_config.fd_only_maximum == true) {
					fd_result_t faceinfo_max;
					faceinfo_max = results[max_size_id];
					face_infos.emplace_back(faceinfo_max);
				} else {
					face_infos = results;
				}
			}

			if (do_face_reg) {
				//Need do face qulity check.
				fdfr.extract(frame,face_infos,features_list);
				if (features_list.size() == 1) {
					uint32_t id = repo.add(face_reg_name, features_list[0].face_feature.features);
					BM_FACE_LOG(LOG_DEBUG_NORMAL, cout << "Add face identity, id = "<< dec << id << endl);
				}
				do_face_reg = 0;
			}

			if (edb_config.do_fr > 0) {
				cv:Mat frame_img = frame.clone();

				if (repo.id_list().size() == 0) {
					BM_FACE_LOG(LOG_DEBUG_ERROR,
						cout << "Repo is NULL, Please check !!" << endl);
					edb_config.do_fr = 0;
				} else {
					fdfr.extract(frame,face_infos,features_list);
					face_result.resize(face_infos.size());

					for (int i = 0; i < face_infos.size(); i++) {
						auto match_result = repo.match(features_list[i].face_feature.features, sim_threshold);

						BM_FACE_LOG(LOG_DEBUG_USER_3,
							cout << "match_result: " << match_result.matched
							<< " " << match_result.id << " " << match_result.score << endl);

						face_result[i] = gen_fr_result(face_infos[i], features_list[i]);
						//memset(face_result[i].name.data(), 0, face_result[i].name.size());
						if(match_result.matched == true) {
							strncpy(face_result[i].name.data(),
								repo.get_match_name(match_result).data(), face_result[i].name.size());
						} else {
							strncpy(face_result[i].name.data(), "unknown", sizeof("unknown"));
						}

						float x1 = face_result[i].face.bbox.x1;
						float y1 = face_result[i].face.bbox.y1;
						float x2 = face_result[i].face.bbox.x2;
						float y2 = face_result[i].face.bbox.y2;
						cv::rectangle(frame, cv::Point(x1, y1),	cv::Point(x2, y2), cv::Scalar(255, 255, 0), 3, 8, 0);

						int pos_x = std::max(int(x2 + 5), 0);
						int pos_y = std::max(int(y1 - 10), 0);
						cv::putText(frame, face_result[i].name.data(),
							cv::Point(pos_x, pos_y), cv::FONT_HERSHEY_PLAIN, 4.0, CV_RGB(255,255,0), 2.0);

						face_pts_t facePts = face_result[i].face.face_pts;
						for (int j = 0; j < 5; j++) {
							cv::circle(frame, cv::Point(facePts.x[j], facePts.y[j]),
								1, cv::Scalar(255, 255, 0), 8);
						}

						if (edb_config.record_image == true) {
							strStream << "echo \"" << edb_config.do_fr << ": "
								<< (strcmp(face_result[i].name.data(), person_name.c_str())?"Fail":"Pass")\
								<< "| name: " << face_result[i].name.data()\
								<< "| score: " << match_result.score\
								<< "\" >> " << (edb_config.record_path+"/"+person_name)+"/"+person_name+".txt";
							cv::Mat test;
							//cout << "txt: " << strStream.str() << endl;
							BmImageRecord(test, "", strStream.str());
						}
					}
					if (edb_config.record_image == true) {
						string pic_name = edb_config.record_path+"/" +	person_name+"/" + person_name + "_" + to_string(edb_config.do_fr)+"_ori.png";
						//cout << "pic: " << pic_name << endl;
						BmImageRecord(frame_img, pic_name, "");
						pic_name = edb_config.record_path+"/" + person_name+"/" + person_name + "_" + to_string(edb_config.do_fr)+".png";
						BmImageRecord(frame, pic_name, "");
					}
					if (edb_config.do_fr < 10000)
						edb_config.do_fr --;
				}
			
			} else {
				if ((edb_config.camera_capture_mode) && (edb_config.record_image == true)) {
					string pic_name = edb_config.record_path + "/" +
						s_time + "_frame_"+ to_string(fram_num++) + ".png";

					cout << "name : " << pic_name << endl;
					//cv::imwrite(_name, frame);
					BmImageRecord(frame, pic_name, "");
				}

				for (int i = 0; i < face_infos.size(); i++) {
					float x1 = face_infos[i].bbox.x1;
					float y1 = face_infos[i].bbox.y1;
					float x2 = face_infos[i].bbox.x2;
					float y2 = face_infos[i].bbox.y2;
					cv::rectangle(frame, cv::Point(x1, y1),	cv::Point(x2, y2), cv::Scalar(255, 255, 0), 3, 8, 0);

					if (edb_config.do_facepose == true) {
						cv::Rect rect(x1, y1, x2, y2);
			            if (rect.x < 0) rect.x = 0;
			            if (rect.y < 0) rect.y = 0;
			            if (rect.x + rect.width >= frame.cols) rect.width = frame.cols - rect.x;
			            if (rect.y + rect.height >= frame.rows) rect.height = frame.rows - rect.y;
			            cv::Mat cropped_img = frame(rect).clone();
						BmFaceFacePose(cropped_img);
					}
					
					face_pts_t facePts = face_infos[i].face_pts;
					for (int j = 0; j < 5; j++) {
						cv::circle(frame, cv::Point(facePts.x[j], facePts.y[j]),
							1, cv::Scalar(255, 255, 0), 8);
					}
				}
			}
		}

		PERFORMACE_TIME_MESSURE_START(start_time);
		BmFaceDisplay(frame);
		PERFORMACE_TIME_MESSURE_END(end_time);
	}

	extern int fd_spidev;
	close(fd_spidev);
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


int CliCmdCamera(int argc,char *argv[])
{
	if (argc == 1) {
		cout<<"Usage: camera [config|open|stop]"<<endl;
		return 0;
	} else if(argc == 2) {
		if (!strcmp(argv[1],"open")) {
			cv::Mat testFrame;
			if (edb_config.source_type == "cam") 
				BmCameraGetFrame(testFrame);
			pthread_create(&bmface_tid, NULL, BmFaceThread,NULL);
			if ((edb_config.record_image == true) || (edb_config.fs_debug_frame_record_real == true)
					|| (edb_config.fs_debug_frame_record_fake == true)) {
				pthread_create(&bmpicrd_tid, NULL, BmPicRecordThread,NULL);
			}
			
			//cout<<"Open uvc camera ."<<endl;
			return 0;
		} else if(!strcmp(argv[1],"stop")) {
			cout<<"Stop uvc camera ."<<endl;
			return 0;
		}
	} else if(argc == 3) {
		if (!strcmp(argv[1],"config")) {
			return 0;
		}
	}

	cout<<"Input Error !!"<<endl;
	return -1;
}




int CliCmdRecordPath(int argc, char* argv[])
{
	if (argc == 2) {
		edb_config.record_path = argv[1];
	}
	cout<<"Test result will in: "<<edb_config.record_path<<" ."<<endl;
	return 0;
}

int CliCmdTestFr(int argc,char *argv[])
{
	char _cmd[200];
	int value;

	if (argc == 1) {
		cout<<"Usage: testfr [num] [name]"<<endl;
		return 0;
	} else if(argc == 3) {
		value = atoi(argv[1]);
		person_name = argv[2];

		sprintf(_cmd,"mkdir -p %s",(edb_config.record_path+"/"+person_name).c_str());
		cout<<"system cmd :"<<_cmd<<endl;
		system(_cmd);
		sprintf(_cmd,"rm %s/*",(edb_config.record_path+"/"+person_name).c_str());
		cout<<"system cmd :"<<_cmd<<endl;
		system(_cmd);
		sprintf(_cmd,"touch %s/%s.txt",(edb_config.record_path+"/"+person_name).c_str(),person_name.c_str());
		cout<<"system cmd :"<<_cmd<<endl;
		system(_cmd);

		edb_config.do_fr = value;
	}

	cout<<" face detect is :"<<(edb_config.do_fr?"Enable":"Disable")<<" ."<<endl;
	return 0;
}

int BmCliCmdThresholdValue(int argc, char *argv[])
{
	if(argc == 1){
		cout<<"Usage: threshold [get|set]"<<endl;
		return 0;
	} else if(argc == 2) {
		if (!strcmp(argv[1],"get")) {
			cout<<"sim_threshold = "<<sim_threshold<<" ."<<endl;
			return 0;
		}
	} else if(argc == 3) {
		if (!strcmp(argv[1],"set")) {
			sim_threshold = strtof(argv[2],NULL);
			cout<<"sim_threshold = "<<sim_threshold<<" ."<<endl;
			return 0;
		}
	}

	cout<<"Input Error !!"<<endl;
	return -1;
}


int CliCmdFaceAlgorithm(int argc, char *argv[])
{
	if (argc == 1) {
		cout<<"Usage: algorithm [change]"<<endl;
	} else if(argc == 2) {
		if (!strcmp(argv[1],"change")) {
			if(fd_algo == FD_TINYSSH) {
				fd_algo = FD_MTCNN_NATIVE;
			} else {
				fd_algo = FD_TINYSSH;
			}
			cout<<"You have changed bmiva face algorithm, please stop uvc and open again !"<<endl;
		}
	}
	cout<<"Current face algorithm is : "<<((fd_algo==FD_TINYSSH)?"tiny_ssh":"mtcnn")<<" ."<<endl;
	return 0;
}

int CliCmdDoFaceRegister(int argc,char *argv[])
{
	if (argc == 1) {
		cout<<"Usage: facereg [name]"<<endl;
		return 0;
	} else if(argc == 2) {
		cout<<"uvc_tid : "<<(int)bmface_tid<<endl;
		if (!bmface_tid) {
			cout<<"Please open camera !!"<<endl;
		} else {
			face_reg_name = argv[1];
			do_face_reg = 1;
		}
	}
	return 0;
}


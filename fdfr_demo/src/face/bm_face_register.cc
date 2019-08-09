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
#include "bm_camera.h"


Repository bm_repo("/system/data/bm_repository");


bool StrCmpIgnoreCases(const string &a, const string &b) {
	int i,j;
	if(a.length() != b.length()) {
		return false;
	} else {
		for (i=0; i<a.length(); i++) {
			if((a[i] == b[i]) || (abs(a[i] - b[i]) == 32)) {
				;
			} else
				return false;
		}
	}
	return true;
}

string  BmGetRegisterName(const char *p_name)
{
	#define LEN 100
	char __name[LEN];
	string name;
	char a;
	int i;
	for (i=0; i < (LEN-1); i++) {
		a = *(p_name + i);
		//printf("%d, a = %c \n",i,a);
		if ((a == ' ') || (a == '-') || (a == '.') || (a == '_')) {
			break;
		}
		else
		{
			__name[i]=a;
		}
	}
	__name[i]=0;
	//printf("__name : %s \n",__name);
	name.assign(__name);
	//cout<<"name: "<<name.c_str()<<endl;
	return name;
}


int _BmFaceAddIdentity(cv::Mat &frame, string &name, size_t id = 0, repo_opts mode = REPO_ADD);

int _BmFaceAddIdentity(cv::Mat &frame, string &name, size_t id , repo_opts mode)
{
	vector<face_info_t> faceInfo;
	vector<face_features_t> features_list;
	if(BmFaceSetupFdFrModel() != 0)
		return -1;

	bm_fdfr->detect(frame,faceInfo);

	std::sort(faceInfo.begin(), faceInfo.end(), [](face_info_t &a, face_info_t &b) {
		return (a.bbox.x2 - a.bbox.x1 + 1) * (a.bbox.y2 - a.bbox.y1 + 1)
				> (b.bbox.x2 - b.bbox.x1 + 1) * (b.bbox.y2 - b.bbox.y1 + 1);
	});
	printf("\ndetect %d faces\n", int(faceInfo.size()));

	#if 1
	float x1 = faceInfo[0].bbox.x1;
	float y1 = faceInfo[0].bbox.y1;
	float x2 = faceInfo[0].bbox.x2;
	float y2 = faceInfo[0].bbox.y2;
	cv::Mat cropped_img = frame(cv::Range(std::max<int>(0, y1), std::min<int>(y2, frame.size[0] - 1)),
                                                cv::Range(std::max<int>(0, x1), std::min<int>(x2, frame.size[1] - 1)));
	if (BmFaceFacePoseCheck(cropped_img) == -1) {
		return -1;
	}
	#endif

	bm_fdfr->extract(frame,faceInfo,features_list);
	uint32_t face_id = 0;
	if (mode == REPO_ADD) {
		face_id = bm_repo.add(name, features_list[0].face_feature.features);
	}else if (mode == REPO_UPDATE) {
		face_id = id;
		bm_repo.set(face_id, name, features_list[0].face_feature.features);
	}
	BM_FACE_LOG(LOG_DEBUG_NORMAL, cout << "Repo size: " << bm_repo.id_list().size() << endl);

	#if 1
	cv::rectangle(frame, cv::Point(x1, y1),	cv::Point(x2, y2), cv::Scalar(0, 0, 255), 3, 8, 0);
	#endif

	return face_id;
}

int BmFaceAddIdentity(cv::Mat &frame, string &name)
{
	return _BmFaceAddIdentity(frame, name);
}

int BmFaceUpdateIdentity(cv::Mat &frame, string &name, size_t id)
{
	return _BmFaceAddIdentity(frame, name, id, REPO_UPDATE);
}


int BmFaceRegister(int src_mode, string &source_path, size_t face_id)
{
	vector<string> all_files;
	if (src_mode == BM_FACE_REG_USE_PICTURE) {
		all_files.emplace_back(source_path);
	}
	else if(src_mode == BM_FACE_REG_USE_FOLDER) {
		BmListAllFiles(source_path, all_files);
	}

	for(int i; i < all_files.size(); i++) {
		cv::Mat frame;

		BM_FACE_LOG(LOG_DEBUG_NORMAL, cout << all_files[i] << " is a file." << endl);

		if (access(all_files[i].c_str(),0) == -1) {
			cout<<"File: \""<<all_files[i]<<"\""<<" does not exist !!"<<endl;
			return -1;
		}
		string suffixStr = all_files[i].substr(all_files[i].find_last_of('.') + 1);

		if (StrCmpIgnoreCases(suffixStr,"jpg") || StrCmpIgnoreCases(suffixStr,"jpeg")
				|| StrCmpIgnoreCases(suffixStr,"png") || StrCmpIgnoreCases(suffixStr,"bmp")) {
			frame = cv::imread(all_files[i]);

			string reg_name = BmGetRegisterName((all_files[i].substr(all_files[i].find_last_of('/')+1)).c_str());
			if (face_id == 0){
				BmFaceAddIdentity(frame, reg_name);
			} else {
				BmFaceUpdateIdentity(frame, reg_name, face_id);
			}

		}
	}
	return 0;
}


int BmFaceShowRepoList(void)
{
	std::vector<size_t> id_table;
	id_table = bm_repo.id_list();

	cout << "repo size = " << id_table.size() << endl;
	for (int i=0; i < id_table.size() ; i++) {
		size_t id = id_table[i];
		cout << "id = " << id << " , name = " << bm_repo.get_name(id).value_or("") << endl;
	}

	return 0;
}

int BmFaceRemoveIdentity(const size_t id)
{
	bm_repo.remove(id);
	return 0;
}

int BmFaceRemoveAllIdentities(void)
{
	bm_repo.remove_all();
	return 0;
}

int CliCmdDoFaceRegister(int argc,char *argv[])
{
	string face_reg_name;
	if (argc == 1) {
		cout<<"Usage: facereg [name]"<<endl;
		return 0;
	} else if(argc == 2) {
		if (bmface_tid) {
			cout<<"Please stop camera first!"<<endl;
			return 0;
		}
		if (bm_testframe.empty()) {
			cout << "Please get frame first." << endl;
			return 0;
		}

		face_reg_name = argv[1];
		BmFaceAddIdentity(bm_testframe, face_reg_name);
		BmFaceDisplay(bm_testframe);
	}
	return 0;
}


int CliCmdRepoManage(int argc,char *argv[])
{
	if (argc == 1) {
		cout<<"Usage: repo [list/delete/deleteall]"<<endl;
		return 0;
	} else if (argc == 2) {
		if (!strcmp(argv[1],"list")) {
			BmFaceShowRepoList();
		} else if (!strcmp(argv[1],"deleteall")) {
			BmFaceRemoveAllIdentities();
		}
	} else if (argc == 3) {
		if (!strcmp(argv[1],"delete")) {
			int face_id = atoi(argv[2]);
			if (face_id > bm_repo.id_list().size()) {
				cout << "Invalid face id: " << face_id << endl;
				return -1;
			}
			BmFaceRemoveIdentity(face_id);
		}
	}
	return 0;
}







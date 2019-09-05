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

#ifndef __FACE_DET_H__
#define __FACE_DET_H__

#include "fdfr.hpp"
#include "face_impl.hpp"

#define FACE_RECOG_IMG_W 112
#define FACE_RECOG_IMG_H 112
#define MAX_NAME_LEN 256


#define  BM_FACE_LOG(x,fmt...) \
		do{\
			if(x <= bmface_log_level)\
			{\
			    fmt;\
			}\
		}while(0)

#define BM_FACE_REG_USE_PICTURE (1)
#define BM_FACE_REG_USE_FOLDER (2)

enum repo_opts
{
	REPO_NULL,
	REPO_ADD,
	REPO_DELETE,
	REPO_UPDATE,
	REPO_LIST,
	REPO_DELETE_ALL,
};

extern int bmface_log_level;
extern vector<vector<string>> detector_models;
extern vector<vector<string>> extractor_models;
extern Repository bm_repo;
extern unique_ptr<FDFR> bm_fdfr;
extern pthread_t bmface_tid;


int BmFaceDisplayInit(void);
int BmFaceDisplay(cv::Mat &frame);
int BmFaceSetupFdFrModel(void);
//int BmFaceSetupAfsClassify(void);
//bool BmFaceRunAfs(cv::Mat &image, face_info_t &faceInfo);
int BmFaceRegister(int src_mode, string &source_path, size_t face_id = 0);
int BmFaceAddIdentity(int mode, string &source_path, size_t face_id);
int BmFaceShowRepoList(void);
int BmFaceRemoveIdentity(const size_t id);
int BmFaceUpdateIdentity(string &source_path, size_t id);
int BmFaceRemoveAllIdentities(void);
int BmFaceFacePoseCheck(const cv::Mat &face_image);
int BmImageRecord(const cv::Mat &frame, const string &name, const string &info);




#endif

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

extern int bmface_log_level;
extern vector<vector<string>> detector_models;
extern vector<vector<string>> extractor_models;
extern face_algorithm_t fd_algo;
extern Repository repo;

int BmFaceDisplayInit(void);
int BmFaceDisplay(cv::Mat &frame);
std::unique_ptr<AntiFaceSpoofing> BmFaceSetupSpoofing(void);
int BmFaceRunSpoofing(std::unique_ptr<AntiFaceSpoofing> &inst, cv::Mat &image,vector<face_info_t> &faceInfo);
void BmfaceDoFaceSpoofing(int mode,string &source_path);
int BmFaceAddIdentity(int mode, string &source_path, size_t face_id);
int BmFaceShowRepoList(void);
int BmFaceRemoveIdentity(const size_t id);
int BmFaceUpdateIdentity(string &source_path, const size_t id);
int BmFaceRemoveAllIdentities(void);
int BmFaceFacePose(const cv::Mat &face_image);
int BmImageRecord(const cv::Mat &frame, const string &name, const string &info);




#endif

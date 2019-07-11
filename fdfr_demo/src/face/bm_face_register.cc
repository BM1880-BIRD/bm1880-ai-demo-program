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


Repository repo("/system/data/repository");

#if 0
vector<vector<shared_ptr<face::quality::QualityChecker>>> quality_checkers;

static bool InitFaceCheckersFromConfig(const ConfigFile &config)
{
    size_t checker_count = config.require<size_t>("face_checker_count");
    vector<vector<string>> checkers(checker_count);
    set<string> subjects;

    for (size_t i = 0; i < checker_count; i++) {
        checkers[i] = config.require<vector<string>>("face_checker_" + to_string(i), "subjects");
        for (auto &subject : checkers[i]) {
            subjects.insert(subject);
        }
    }

    quality_checkers.clear();
    quality_checkers.resize(checker_count);

    for (size_t i = 0; i < checker_count; i++) {
        vector<shared_ptr<face::quality::QualityChecker>> checker;
        string section_name = "face_checker_" + to_string(i);

        for (auto &subject : checkers[i]) {
            if (subject == "facepose") {
                checker.emplace_back(make_shared<face::quality::FaceposeCheck>(config.require<double>(section_name, "facepose_threshold"), facepose));
            } else if (subject == "face_boundary") {
                checker.emplace_back(make_shared<face::quality::FaceBoundaryCheck>());
            } else if (subject == "landmark_boundary") {
                checker.emplace_back(make_shared<face::quality::LandmarkBoundaryCheck>());
            } else if (subject == "skin_area") {
                checker.emplace_back(make_shared<face::quality::SkinAreaCheck>(config.require<double>(section_name, "skin_area_threshold")));
            } else if (subject == "clarity") {
                checker.emplace_back(make_shared<face::quality::FaceClarityCheck>(config.require<double>(section_name, "clarity_threshold")));
            } else if (subject == "resolution") {
                checker.emplace_back(make_shared<face::quality::FaceResolutionCheck>(config.require<size_t>(section_name, "resolution_threshold")));
            } else if (subject == "antispoofing") {
                checker.emplace_back(make_shared<face::quality::AntiSpoofingCheck>(antispoofing));
            } else {
                fprintf(stderr, "\"%s\" is not a supported face quality checker.\n", subject.data());
                return false;
            }
        }

        quality_checkers[i] = move(checker);
    }

    return true;
}


int BmFaceQualityCheck(bmlib_handle_t handle, cv::Mat &full_image, const bm_face_info_t *face_info, int checker_id, int *result)
{
    bm_face_context_t *ctx = static_cast<bm_face_context_t*>(handle);
    image_t &img = *((image_t*)(image->internal_data));
    qnn::vision::face_info_t face;
    //convert_face(face, face_info);
    cv::Mat full_image = img;
    cv::Mat face_image;
    try {
        face_image = full_image(cv::Range(std::max<int>(0, face.bbox.y1), std::min<int>(face.bbox.y2, full_image.size[0] - 1)),
                                cv::Range(std::max<int>(0, face.bbox.x1), std::min<int>(face.bbox.x2, full_image.size[1] - 1)));
    } catch (cv::Exception &e) {
        return -1;
    }

    if (checker_id < 0 || ctx->checkers.size() <= checker_id) {
        return -1;
    }

    *result = 0;
    for (auto &checker : ctx->checkers[checker_id]) {
        if (!checker->check(full_image, face_image, face)) {
            *result = 1 << checker->code();
            break;
        }
    }

    return 0;
}
#endif

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


int BmFaceAddIdentity(int mode, string &source_path, size_t face_id)
{
	FDFR fdfr(fd_algo, detector_models[fd_algo], FR_BMFACE, extractor_models[0]);


	vector<string> all_files;
	if (mode == 1) {
		all_files.emplace_back(source_path);
	}
	else if(mode == 2) {
		BmListAllFiles(source_path, all_files);
	}

	for(int i; i < all_files.size(); i++) {
		cv::Mat frame;
		vector<fd_result_t> results;
		vector<face_features_t> features_list;

		BM_FACE_LOG(LOG_DEBUG_NORMAL, cout << all_files[i] << " is a file." << endl);

		if (access(all_files[i].c_str(),0) == -1) {
			cout<<"File: \""<<all_files[i]<<"\""<<" does not exist !!"<<endl;
			return -1;
		}
		string suffixStr = all_files[i].substr(all_files[i].find_last_of('.') + 1);

		if (StrCmpIgnoreCases(suffixStr,"jpg") || StrCmpIgnoreCases(suffixStr,"jpeg")
				|| StrCmpIgnoreCases(suffixStr,"png") || StrCmpIgnoreCases(suffixStr,"bmp")) {
			frame = cv::imread(all_files[i]);
			fdfr.detect(frame,results);
			fdfr.extract(frame,results,features_list);
			if (features_list.size() == 1) {
				string reg_name = BmGetRegisterName((all_files[i].substr(all_files[i].find_last_of('/')+1)).c_str());
				if(mode == 3)
				{
					if(face_id <= 0)
						return -1;
					repo.set(face_id, reg_name, features_list[0].face_feature.features);
				}
				else
				{
					uint32_t id = repo.add(reg_name, features_list[0].face_feature.features);
					BM_FACE_LOG(LOG_DEBUG_NORMAL, cout << "Add face identity, "<< reg_name << ", id = "<< dec << id << endl);
				}
			} else {
				BM_FACE_LOG(LOG_DEBUG_ERROR, cout << "Reject: This pic has "<< dec << features_list.size() << "faces!"<< endl);
			}
		}
	}
	return 0;
}


int BmFaceShowRepoList(void)
{
	std::vector<size_t> id_table;
	id_table = repo.id_list();

	cout << "repo size = " << id_table.size() << endl;
	for (int i=0; i < id_table.size() ; i++) {
		size_t id = id_table[i];
		cout << "id = " << id << " , name = " << repo.get_name(id).value_or("").data() << endl;
	}

	return 0;
}


int BmFaceUpdateIdentity(string &source_path, const size_t id)
{
	return BmFaceAddIdentity(3, source_path, id);
}

int BmFaceRemoveIdentity(const size_t id)
{
	repo.remove(id);
	return 0;
}

int BmFaceRemoveAllIdentities(void)
{
	repo.remove_all();
	return 0;
}



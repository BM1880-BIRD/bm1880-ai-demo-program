#include <memory>
#include <vector>
#include <string>

#include "qnn.hpp"


using namespace std;
using namespace cv;
using namespace qnn::vision;




int BmFaceFacePose(const cv::Mat &face_image)
{
	std::vector<std::vector<float>> detect_result;
	Facepose detector_("/system/data/bmodel/Face/Facepose/light_rpoly_lmp_iter_75000_w_scale.bmodel", NULL);


	try {
	    detector_.Detect({(cv::Mat)face_image}, detect_result);
	} catch (cv::Exception &e) {
	    throw std::runtime_error(std::string("cv::Exception: ") + e.what());
	}

	const size_t feature_offset = 136;
	const size_t feature_count = 3;

	for (size_t i = 0; i < feature_count; i++) {
	    double output = abs(detect_result[0][feature_offset + i] * 90);
		cout << "BmFaceFacePose: " << i << " = " << output << endl;
	}

	return 0;
}



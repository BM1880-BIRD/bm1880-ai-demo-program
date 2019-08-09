#include <memory>
#include <vector>
#include <string>

#include "qnn.hpp"


using namespace std;
using namespace cv;
using namespace qnn::vision;



#if 0
int BmFaceFacePose(const cv::Mat &face_image)
{
	std::vector<std::vector<float>> detect_result;
	Facepose detector_("/system/data/bmodel/facepose.bmodel", NULL);

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
#endif

int BmFaceFacePoseCheck(const cv::Mat &face_image)
{
	std::vector<std::vector<float>> detect_result;
	Facepose detector_("/system/data/bmodel/facepose.bmodel", NULL);

	try {
	    detector_.Detect({(cv::Mat)face_image}, detect_result);
	} catch (cv::Exception &e) {
	    throw std::runtime_error(std::string("cv::Exception: ") + e.what());
	}

	const size_t feature_offset = 136;
	//const size_t feature_count = 3;
	#if 0
	for (size_t i = 0; i < feature_count; i++) {
	    float output = abs(detect_result[0][feature_offset + i] * 90);
		cout << "BmFaceFacePoseCheck: " << i << " = " << output << endl;
		if (output > 10) {
			return -1;
		}
	}
	#endif
	float output0 = abs(detect_result[0][feature_offset + 0] * 90);
	float output1 = abs(detect_result[0][feature_offset + 1] * 90);
	float output2 = abs(detect_result[0][feature_offset + 2] * 90);

	cout << "BmFaceFacePoseCheck: " << 0 << " = " << output0 << endl;
	cout << "BmFaceFacePoseCheck: " << 1 << " = " << output1 << endl;
	cout << "BmFaceFacePoseCheck: " << 2 << " = " << output2 << endl;

	if ((output0 > 10) || (output1 > 10) || (output2 > 10)) {
		return -1;
	}

	return 0;
}




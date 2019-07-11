#include "usb_output.hpp"

#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <cmath>
#include <sstream>
#include "log_common.h"

#define DEVICE_FILENAME_TX		"/dev/bmusb_tx"
#define WRITE_CHUNK_SIZE		1024

using namespace std;

USBOutput::USBOutput() {
	fd_write = open(DEVICE_FILENAME_TX, O_RDWR | O_NDELAY);
}

USBOutput::~USBOutput() {
	close();
}

bool USBOutput::is_opened() {
	return (fd_write >= 0);
}

void USBOutput::close() {
	::close(fd_write);
}

/// TODO: timeout for wrtie
bool USBOutput::put(vector<fr_result_t> &&results) {
	string result_str = get_result_json_string(results);

	double json_size = result_str.size();
	int buf_chunk_cnt = (int)(ceil(json_size / WRITE_CHUNK_SIZE));
	int buf_size = buf_chunk_cnt * WRITE_CHUNK_SIZE;
	int total_write_size = 0;

	buf = (char *)malloc(sizeof(char) * buf_size);

	memset(buf, ' ', buf_size);
	snprintf(buf, buf_size, "%s", result_str.c_str());

	LOGD << "start to write, target size = " << buf_size << endl;
	for (int i = 0; i < buf_chunk_cnt; ++i) {
		printf("write %d/%d\n", i+1, buf_chunk_cnt);
		char *buf_start = buf + (WRITE_CHUNK_SIZE * i);
		total_write_size += write(fd_write, buf_start, sizeof(char) * WRITE_CHUNK_SIZE);
	}
	LOGD << "total write size = " << total_write_size << endl;

	free(buf);
}

string USBOutput::get_result_json_string(vector<fr_result_t> &results) {
	ostringstream ss_json;

	ss_json << "{'data':[";

	for (int i = 0; i < results.size(); ++i) {
		auto &result = results[i];

		ostringstream ss_bbox;
		ostringstream ss_pts;
		ostringstream ss_features;

		float x1 = result.face.bbox.x1;
		float y1 = result.face.bbox.y1;
		float x2 = result.face.bbox.x2;
		float y2 = result.face.bbox.y2;
		ss_bbox << "'boxes':[" << x1 << "," << y1 << "," << x2 << "," << y2 << "]";

		face_pts_t pts = result.face.face_pts;
		ss_pts << "'points':[";
		for (int j = 0; j < FACE_PTS; ++j) {
			if (0 < j) {
				ss_pts << ",";
			}
			ss_pts << pts.x[j] << "," << pts.y[j];
		}
		ss_pts << "]";

		ss_features << "'features':[";
		for (int j = 0; j < result.features.size(); ++j) {
			if (0 < j) {
				ss_features << ",";
			}
			ss_features << result.features[j];
		}
		ss_features << "]";

		ss_json << "{" << ss_bbox.str() << "," << ss_pts.str() << "," << ss_features.str() << "}";

		if (i != (results.size()-1)) {
			ss_json << ",";
		}
	}

	ss_json << "]}";

	return ss_json.str();
}
#include "usb_input.hpp"
#include <opencv2/opencv.hpp>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <string.h>
#include <errno.h>
#include <sys/time.h>
#include <unistd.h>
#include <stdlib.h>
#include <vector>
#include "logging.hpp"

using namespace std;

#define DEVICE_FILENAME_RX		"/dev/bmusb_rx"
#define BUF_SIZE				4000000
#define READ_CHUNK_SIZE			1024

/* packet format */
/* marker | data type (4 bytes) | date length (4 bytes) | data ..... (data length bytes)) | */
const string start_marker = "\r\n\r\n";
const int header_type_size = 4;
const int header_len_size = 4;
const int header_size = start_marker.size() + header_type_size + header_len_size;

static inline bool is_header(char *buf)
{
	for (int i = 0; i < start_marker.size(); ++i) {
		if (buf[i] != start_marker[i]) {
			return false;
		}
	}

	return true;
}

USBCaptureSource::USBCaptureSource() :
DataSource<cv::Mat>("USBCaptureSource")
{
	fd_read = open(DEVICE_FILENAME_RX, O_RDWR | O_NDELAY);
	read_size = READ_CHUNK_SIZE;
	buf = (unsigned char *)malloc(BUF_SIZE * sizeof(unsigned char));
	curr_buf = buf;
}

USBCaptureSource::~USBCaptureSource() {
	close();
	free(buf);
}

bool USBCaptureSource::is_opened() {
	return (fd_read >= 0);
}

void USBCaptureSource::close() {
	::close(fd_read);
}

void USBCaptureSource::drop_header_if_needed(int new_header_pos) {
	if (headers.empty()) {
		return;
	}

	int data_end = headers.front().data_begin + headers.front().data_size;

	if (new_header_pos < data_end) {
		headers.pop_front();
	}
}

mp::optional<cv::Mat> USBCaptureSource::get() {
	cv::Mat image;

	while (true) {
		int ret_read = read(fd_read, (unsigned char *)curr_buf, sizeof(unsigned char) * read_size);

        LOGD << "read: ret / target = " << ret_read << "/" << read_size << endl;

		if (ret_read < 0) {
			usleep(100000);
            LOGE << "read ret = " << ret_read << endl;
			continue;
		}

		int curr_pos = curr_buf - buf;
		int curr_buf_end = curr_pos + ret_read;
		int rest_buf_size = BUF_SIZE - curr_buf_end;
		int search_begin = curr_pos;
		int search_end = curr_pos + (ret_read - header_size + 1);

		if (ret_read != read_size) {
            LOGW << "current buf pos =" << curr_pos << ", read size = " << ret_read << endl;
		}

		read_size = READ_CHUNK_SIZE;

		for (int i = search_begin; i < search_end; ++i) {
			if (is_header((char *)(buf+i))) {
				int diff = i%READ_CHUNK_SIZE;
				if (diff != 0) {
					read_size = READ_CHUNK_SIZE*2 - diff;
				}
                LOGD << "head is at " << i << ", distance from boundary is " << diff << endl;

				drop_header_if_needed(i);
				parse_header(buf+i);
			}
		}

		if (!headers.empty()) {
			int data_end = headers.front().data_begin + headers.front().data_size;

			if (curr_buf_end >= data_end) {
				bool data_ready = handle_data(image);

				headers.pop_front();

				if (data_ready) {
					goto OUT;
				}
			}
		}

		if (rest_buf_size > READ_CHUNK_SIZE) {
			curr_buf += ret_read;
		} else {
			unsigned char *copy_src = buf + headers.front().data_begin;
			int copy_size = (curr_buf + ret_read) - copy_src;

			memcpy(buf, copy_src, copy_size);
			curr_buf = buf + copy_size;
			headers.front().data_begin = 0;
		}
	}

OUT:

	return {mp::inplace_t(), image};
}

void USBCaptureSource::parse_header(unsigned char *buf_header) {
	USBCaptureHeader header;

	header.data_type = static_cast<DATA_TYPE>(*((int*)(buf_header + start_marker.size())));
	header.data_size = *((int*)(buf_header + start_marker.size() + header_type_size));
	header.data_begin = (buf_header - buf) + header_size;

	headers.push_back(header);

    LOGD << "header: type=" << header.data_type << ", size=" << header.data_size
        << ", data:[" << header.data_begin << "," << header.data_begin+header.data_size << "]" << endl;
}

bool USBCaptureSource::handle_data(cv::Mat &image) {
	bool data_ready = false;

	switch (headers.front().data_type) {
	case DATA_TYPE_IMAGE:
		data_ready = get_image_from_buf(image);
		break;
	default:
		break;
	}

	return data_ready;
}

bool USBCaptureSource::get_image_from_buf(cv::Mat &image) {
	int img_begin = headers.front().data_begin;
	int img_end = headers.front().data_begin + headers.front().data_size;
	int img_size = headers.front().data_size;

	LOGD << "## full jpeg size = " << img_size << ", buf range = [" << img_begin << ", " << img_end << "]" << endl;

	int i = img_begin;
	if ((buf[i] != 0xFF) || (buf[i+1] != 0xD8)) {
		LOGE << "[error] jpeg head not found." << endl;
		return false;
	}
	i = img_end - 2;
	if ((buf[i] != 0xFF) || (buf[i+1] != 0xD9)) {
		LOGE << "[error] jpeg tail not found." << endl;
		return false;
	}

	vector<unsigned char> img_data(buf + img_begin, buf + img_end);
	image = cv::imdecode(img_data, CV_LOAD_IMAGE_COLOR);

	if ((image.size().width <= 0) || (image.size().height <= 0)) {
		LOGE << "jpeg img decode failed." << endl;
		return false;
	}

	LOGD << "jpeg decode ok, w=" << image.size().width << ", h=" << image.size().height << endl;

	return true;
}
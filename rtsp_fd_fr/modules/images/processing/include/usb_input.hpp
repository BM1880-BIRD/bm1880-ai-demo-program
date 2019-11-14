#pragma once

#include "data_source.hpp"
#include "image.hpp"

namespace cv {
	class Mat;
}

class USBCaptureSource : public DataSource<cv::Mat> {
private:
	enum DATA_TYPE {
		DATA_TYPE_IMAGE = 1,
	};

	struct USBCaptureHeader {
		DATA_TYPE data_type;
		int data_size;
		int data_begin;
	};

public:
    USBCaptureSource();
    ~USBCaptureSource();

	bool is_opened() override;
	void close() override;
    mp::optional<cv::Mat> get() override;

private:
	void drop_header_if_needed(int new_header_pos);
	void parse_header(unsigned char *buf_header);
	bool handle_data(cv::Mat &image);
	bool get_image_from_buf(cv::Mat &image);

private:
	int fd_read;
	int read_size;
	unsigned char *buf;
	unsigned char *curr_buf;
	std::list<USBCaptureHeader> headers;
};
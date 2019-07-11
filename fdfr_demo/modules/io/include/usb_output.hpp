#pragma once

#include "data_sink.hpp"
#include "face_api.hpp"
#include <string>
#include <vector>

class USBOutput : public DataSink<std::vector<fr_result_t>> {
public:
    USBOutput();
    ~USBOutput();

	bool is_opened();
    void close() override;
	bool put(std::vector<fr_result_t> &&results) override;

private:
	std::string get_result_json_string(std::vector<fr_result_t> &results);

private:
	int fd_write;
	char *buf;
};
#pragma once

#include <vector>
#include <string>

typedef struct {
    float x1;
    float y1;
    float x2;
    float y2;
    int classes;
    float prob;
    std::string label;
} object_detect_rect_t;

void remote_yolo_init();
void remote_yolo_send(std::vector<unsigned char> &&input_jpg);
int remote_yolo_receive(std::vector<unsigned char> &output_jpg, std::vector<object_detect_rect_t> &result);
void remote_yolo_free();

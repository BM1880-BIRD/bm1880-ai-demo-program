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

enum InferenceModel {
    YOLOV2,
    YOLOV3,
    OPENPOSE
};

void libusb_inference_init();
void libusb_inference_load_model(InferenceModel model);
void libusb_inference_send(InferenceModel model, std::vector<unsigned char> &&input_jpg);
int libusb_inference_receive(std::vector<unsigned char> &output_jpg);
int libusb_inference_receive_object_detect(std::vector<unsigned char> &output_jpg, std::vector<object_detect_rect_t> &result);
void libusb_inference_free();

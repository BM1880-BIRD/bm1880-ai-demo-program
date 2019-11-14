#pragma once

#include <opencv2/opencv.hpp>

#ifdef __cplusplus
extern "C"
{
#endif
    struct FaceSensorParameter
    {
        int SensorType;
        int Sensor_width;
        int Sensor_height;
        int o_RGBType;
        int o_imagewidth;
        int o_imageheigth;
        int o_rotate;
        int o_flip;
    };

    int init_facesensor(struct FaceSensorParameter facesensor, int sensortype);

    int grab_image_color(unsigned char *buffer, int buflen, int sensortype_index, char flag);

#ifdef __cplusplus
}
#endif

bool ZktCameraFam600Init();
int ZktFam600GetFrame(cv::Mat &matFrame);

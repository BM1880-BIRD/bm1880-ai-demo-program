#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include "zkt.hpp"
#include <opencv2/imgcodecs.hpp>

using namespace std;

#define BUF_LEN (640 * 480 * 3)
static int handle_cl = 0;
unsigned char *buffer;

bool ZktCameraFam600Init()
{
    static int HasInit = false;
    struct FaceSensorParameter facesensor;

    if (HasInit == true)
    {
        return true;
    }

    cout << __FUNCTION__ << "  " << __LINE__ << endl;

    buffer = (unsigned char *)malloc(BUF_LEN);
    facesensor.SensorType = 30;
    facesensor.o_flip = 1;
    facesensor.o_imageheigth = 480;
    facesensor.o_imagewidth = 640;
    facesensor.o_RGBType = 1;
    facesensor.o_rotate = 1;
    facesensor.Sensor_height = 480;
    facesensor.Sensor_width = 640;

    handle_cl = init_facesensor(facesensor, 0);
    if (handle_cl < 0)
    {
        cout << "init facesensor failed, handle_cl: " << handle_cl << endl;
        return false;
    }

    HasInit = true;
    return true;
}

int ZktFam600GetFrame(cv::Mat &matFrame)
{
    int ret;
    int len;

    if (ZktCameraFam600Init() == false)
        return -1;

    memset(buffer, 0, BUF_LEN);
    ret = grab_image_color(buffer, BUF_LEN, handle_cl, 0);

    if (ret <= 0)
    {
        cout << "get image failed!ret = " << ret << endl;
        return -1;
    }

    vector<unsigned char> vbuffer(buffer, buffer + ret);

    if (buffer[0] == 0xff && buffer[1] == 0xD8)
    {
        cv::Mat Frame = cv::imdecode(vbuffer, cv::IMREAD_COLOR);
        cv::Rect rect(0, 0, Frame.cols, Frame.rows / 2);
        cv::Mat FrameORI = Frame(rect);
        matFrame = FrameORI;
        cv::resize(FrameORI, matFrame, cv::Size(1280, 720));
    }
    return 0;
}

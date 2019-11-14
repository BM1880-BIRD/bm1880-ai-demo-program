# libusb_inference_client

## Build
Execute under root of rtsp_fd_fr
```
cmake -DPROGRAMS="libusb_inference_client" -DPLATFORM=cmodel . -Bbuild/cmodel
make -C build/cmodel
```
You may change configuration of platform based on requirement.
`cmodel` uses current arcitecture.
You will find `libusb_inference_client.so` under directory `build/cmodel/lib`.

## Headers
`libusb_inference_client.hpp` provides following functions
```
void libusb_inference_init();
void libusb_inference_load_model(InferenceModel model);
void libusb_inference_send(InferenceModel model, std::vector<unsigned char> &&input_jpg);
int libusb_inference_receive(std::vector<unsigned char> &output_jpg);
int libusb_inference_receive_object_detect(std::vector<unsigned char> &output_jpg, std::vector<object_detect_rect_t> &result);
void libusb_inference_free();
```

### `libusb_inference_init()`
Initialize libusb connection

### `libusb_inference_load_model(InferenceModel model)`
Requesting connected edge device to load network model of the specified algorithm.
#### Arguments
> model: currently supported values: `YOLOV2`, `YOLOV3`, `OPENPOSE`

### `libusb_inference_send(InferenceModel model, vector<unsigned char> &&jpg)`
Send jpeg image to inference with on connected edge device.
#### Arguments
> model: currently supported values: `YOLOV2`, `YOLOV3`, `OPENPOSE`

> jpg: jpeg image

### `libusb_inference_receive(std::vector<unsigned char> &output_jpg)`
receive inference result of `OPENPOSE`
#### Arguments
> jpg: jpeg image containing inference result
#### Return Value
> 0 on success, -1 on error

### `libusb_inference_receive_object_detect(std::vector<unsigned char> &output_jpg, std::vector<object_detect_rect_t> &result)`
receive inference result of `YOLOV2` or `YOLOV3`
#### Arguments
> jpg: jpeg image containing inference result

> result: data structure containing result of object detection
#### Return Value
> 0 on success, -1 on error

### `libusb_inference_free()`
closes libusb connection

## Usage
The program must call `libusb_inference_init()` before other actions and must call `libusb_inference_free()` before exiting.

After calling `libusb_inference_init()`, call `libusb_inference_load_model` to request edge device to load network model required.

After loading model, call `libusb_inference_send` to send data, `libusb_inference_receive / libusb_inference_receive_object_detect` to receive result. Response of yolo requests must be received with `libusb_inference_receive_object_detect`.

`src/libusb_inference_client/libusb_inference_client_opencv.cpp` provides an example that gets image and outputs result with opencv.
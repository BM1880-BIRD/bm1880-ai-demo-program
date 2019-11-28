cmake_minimum_required(VERSION 2.8)

set(USB_API_DIR ../../usb_api)
project(darknet_client)
add_executable(darknet_client darknet_client.c ${USB_API_DIR}/host/libusb_comm.c)
target_include_directories(darknet_client PUBLIC ${USB_API_DIR}/host)
target_link_libraries(darknet_client usb-1.0 udev )

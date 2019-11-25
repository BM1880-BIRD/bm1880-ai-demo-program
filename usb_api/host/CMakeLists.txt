cmake_minimum_required(VERSION 2.8)
project(libusb_comm_test)
add_executable(libusb_comm_test libusb_comm_test.c libusb_comm.c)
target_link_libraries(libusb_comm_test usb-1.0 udev)
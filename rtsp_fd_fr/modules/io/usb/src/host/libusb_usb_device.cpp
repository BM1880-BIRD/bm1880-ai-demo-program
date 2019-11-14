#include "usb_device.hpp"
#include <atomic>
#include <thread>
#include <libusb-1.0/libusb.h>
#include <utility>
#include "logging.hpp"

static constexpr unsigned char RX_ENDPOINT = 0x81;
static constexpr unsigned char TX_ENDPOINT = 0x01;
static constexpr int BM_VENDER_ID = 0x30b1;
static constexpr int BM1880_PRODUCT_ID = 0x1003;
static constexpr int INTERFACE_ID = 0;

using namespace std;

namespace usb_io {

namespace detail {

class DeviceImpl {
public:
    DeviceImpl() {
        if (libusb_init(&ctx) < 0) {
            throw std::runtime_error("cannot initialize libusb");
        }

        dev = libusb_open_device_with_vid_pid(ctx, BM_VENDER_ID, BM1880_PRODUCT_ID);

        if (dev == nullptr) {
            libusb_exit(ctx);
            throw std::runtime_error("BM1880 is not plugged");
        }

        libusb_set_auto_detach_kernel_driver(dev, 1);
        int ret;
        if ((ret = libusb_claim_interface(dev, INTERFACE_ID)) != 0) {
            close();
            libusb_exit(ctx);
            throw std::runtime_error("failed claiming interface");
        }

        running.store(true);
    }

    ~DeviceImpl() {
        running.store(false);
        if (dev != nullptr) {
            libusb_release_interface(dev, INTERFACE_ID);
            libusb_close(dev);
            dev = nullptr;
        }
        libusb_exit(ctx);
    }

    bool is_opened() {
        return running.load();
    }

    void close() {
        running.store(false);
    }

    std::pair<bool, size_t> read(unsigned char *buffer, size_t length, int timeout) {
        if (!running.load()) {
            return make_pair(false, 0);
        }

        int actual_length;
        int ret = libusb_bulk_transfer(dev, RX_ENDPOINT, buffer, length, &actual_length, timeout);

        if (ret != LIBUSB_TRANSFER_COMPLETED) {
            LOGD << "usb read returned " << ret << ", " << actual_length << ", " << length;
        }

        return make_pair<bool, size_t>(ret == LIBUSB_TRANSFER_COMPLETED, actual_length);
    }

    std::pair<bool, size_t> write(unsigned char *buffer, size_t length, int timeout) {
        if (!running.load()) {
            return make_pair(false, 0);
        }

        int actual_length;
        int ret = libusb_bulk_transfer(dev, TX_ENDPOINT, buffer, length, &actual_length, timeout);

        if (ret != LIBUSB_TRANSFER_COMPLETED) {
            LOGD << "usb write returned " << ret << ", " << actual_length << ", " << length;
        }

        return make_pair<bool, size_t>(ret == LIBUSB_TRANSFER_COMPLETED, actual_length);
    }

private:
    atomic_bool running;
    libusb_context *ctx;
    libusb_device_handle *dev;
};

} // namespace detail

Device::Device() {
    impl.reset(new detail::DeviceImpl());
}

Device::~Device() {
}

bool Device::is_opened() {
    return impl->is_opened();
}

void Device::close() {
    impl->close();
}

std::pair<bool, size_t> Device::read(unsigned char *buffer, size_t length, int timeout) {
    return impl->read(buffer, length, timeout);
}

std::pair<bool, size_t> Device::write(unsigned char *buffer, size_t length, int timeout) {
    return impl->write(buffer, length, timeout);
}

} // namespace usb_io

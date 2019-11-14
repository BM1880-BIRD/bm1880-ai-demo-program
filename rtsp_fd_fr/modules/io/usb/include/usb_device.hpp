#pragma once

#include <cstddef>
#include <memory>
#include <functional>
#include <utility>

namespace usb_io {

namespace detail {
    class DeviceImpl;
}

/**
 * \class Device
 * Device provides access to libusb communication.
 * The underlying class DeviceImpl has different implementation on different platform.
 *
 * The implementation on bm1880 controls USB IO devices /dev/bmusb_tx and /dev/bmusb_rx.
 *
 * The implementation on other platforms controls USB IO device connected to a bm1880 board with libusb
 */
class Device {
public:
    Device();
    ~Device();

    /// @return true if the device is opened (i.e., not closed)
    bool is_opened();
    /// close the device
    void close();
    /**
     * read data from the device to given buffer with maximum length.
     * extra data that is longer than the buffer is dropped.
     * @return true if success (buffer length equals to data length); false if timeout or other failure
     * @return number of bytes read
     */
    std::pair<bool, size_t> read(unsigned char *buffer, size_t length, int timeout);
    /**
     * write data in the buffer to the device.
     * @return true if success (buffer length equals to written length); false if timeout or other failure
     * @return number of bytes written
     */
    std::pair<bool, size_t> write(unsigned char *buffer, size_t length, int timeout);

private:
    std::unique_ptr<detail::DeviceImpl> impl;
};

}

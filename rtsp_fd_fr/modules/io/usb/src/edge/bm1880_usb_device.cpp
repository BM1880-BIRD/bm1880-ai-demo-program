#include "usb_device.hpp"
#include <atomic>
#include <thread>
#include <queue>
#include <fcntl.h>
#include <unistd.h>
#include <poll.h>
#include <ios>
#include <mutex>
#include <condition_variable>
#include <future>
#include <memory>
#include <chrono>
#include <sys/ioctl.h>
#include <linux/ioctl.h>
#include "logging.hpp"
#include <signal.h>
#include <pthread.h>
#include "mp/optional.hpp"

#define IOC_MAGIC '\x66'

#define IOCTL_VALSET      _IOW(IOC_MAGIC, 0, struct ioctl_arg)
#define IOCTL_VALGET      _IOR(IOC_MAGIC, 1, struct ioctl_arg)
#define IOCTL_VALGET_NUM  _IOR(IOC_MAGIC, 2, int)
#define IOCTL_VALSET_NUM  _IOW(IOC_MAGIC, 3, int)

#define IOCTL_SYNC  0
#define IOCTL_ASYNC 1

#define IOCTL_VAL_MAXNR 3

constexpr const char *RX_ENDPOINT = "/dev/bmusb_rx";
constexpr const char *TX_ENDPOINT = "/dev/bmusb_tx";

using namespace std;

static int setup_fd(int fd) {
    struct ioctl_arg {
        unsigned int reg;
        unsigned int val;
    };
    struct ioctl_arg cmd;

    if (ioctl(fd, IOCTL_VALGET, &cmd) != 0) {
        return -1;
    }
    cmd.val = 0xCC;
    if (ioctl(fd, IOCTL_VALSET, &cmd) != 0) {
        return -1;
    }
    if (ioctl(fd, IOCTL_VALGET, &cmd) != 0) {
        return -1;
    }

    int num = IOCTL_ASYNC;
    if (ioctl(fd, IOCTL_VALSET_NUM, num) != 0) {
        return -1;
    }
    if (ioctl(fd, IOCTL_VALGET_NUM, &num) != 0) {
        return -1;
    }

    return 0;
}

namespace usb_io {

namespace detail {

class DeviceImpl {
    struct endpoint {
        int fd;
        atomic_bool running;
        mutex lock;

        mutex trace_mutex;
        chrono::steady_clock::time_point last_invoke;
        mp::optional<pthread_t> trace_caller_id;
    };

public:
    DeviceImpl() {
        rx.fd = open(RX_ENDPOINT, O_RDONLY | O_NDELAY);
        tx.fd = open(TX_ENDPOINT, O_WRONLY | O_NDELAY);

        if (rx.fd < 0 || tx.fd < 0) {
            close();
            throw ios_base::failure("failed opening usb device");
        }

        if (setup_fd(rx.fd) != 0 || setup_fd(tx.fd) != 0) {
            close();
            throw ios_base::failure("failed ioctl");
        }

        rx.running.store(true);
        tx.running.store(true);
    }

    ~DeviceImpl() {
        close_endpoint(rx);
        close_endpoint(tx);
    }

    bool is_opened() {
        return rx.running.load() && tx.running.load();
    }

    void close() {
        rx.running.store(false);
        tx.running.store(false);
    }

    std::pair<bool, size_t> read(unsigned char *buffer, size_t length, int timeout) {
        return process_event(rx, ::read, POLLIN, buffer, length, timeout);
    }

    std::pair<bool, size_t> write(unsigned char *buffer, size_t length, int timeout) {
        return process_event(tx, ::write, POLLOUT, buffer, length, timeout);
    }

private:
    template <typename func_t>
    static pair<bool, size_t> process_event(endpoint &ep, func_t io_func, int poll_event, unsigned char *buffer, size_t length, int timeout) {
        auto expire_time = chrono::steady_clock::now() + chrono::milliseconds(timeout);
        const char *op = reinterpret_cast<func_t>(::read) == io_func ? "read" : (reinterpret_cast<func_t>(::write) == io_func ? "write" : "unknown_op");

        lock_guard<mutex> locker(ep.lock);
        int transfered = 0;

        {
            lock_guard<mutex> locker(ep.trace_mutex);
            ep.trace_caller_id.emplace(pthread_self());
        }

        while (ep.running.load()) {
            {
                lock_guard<mutex> locker(ep.trace_mutex);
                ep.last_invoke = chrono::steady_clock::now();
            }
            transfered = io_func(ep.fd, buffer, length);
            if (transfered > 0) {
                break;
            } else if (transfered < 0) {
                LOGI << op << " error: " << errno;
                if (errno != EINTR) {
                    break;
                }
            }
            if (timeout > 0 && chrono::steady_clock::now() > expire_time) {
                break;
            }
        }

        {
            lock_guard<mutex> locker(ep.trace_mutex);
            ep.trace_caller_id.reset();
        }

        if (transfered > 0) {
            return {length == transfered, transfered};
        } else {
            return {false, 0};
        }
    }

    void close_endpoint(endpoint &ep) {
        ep.running.store(false);

        unique_lock<mutex> locker(ep.lock);
        if (ep.fd >= 0) {
            fsync(ep.fd);
            ::close(ep.fd);
            ep.fd = -1;
        }
    }

    endpoint rx;
    endpoint tx;
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

}
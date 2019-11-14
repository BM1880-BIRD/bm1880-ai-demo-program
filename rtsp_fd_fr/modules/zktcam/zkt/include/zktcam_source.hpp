#pragma once

#include <atomic>
#include "data_source.hpp"
#include "image.hpp"
#include "json.hpp"

class ZktCameraSource : public DataSource<image_t> {
public:
    explicit ZktCameraSource(const json &);
    ~ZktCameraSource() {}

    mp::optional<image_t> get() override;
    bool is_opened() override;
    void close() override;

private:
    std::atomic<bool> opened;
};
#pragma once

#include <tuple>
#include <string>
#include "data_io.hpp"
#include "mp/optional.hpp"

template <typename... T>
class DataSource : public virtual DataIO {
public:
    using source_type = DataSource<T...>;
    using output_type = std::tuple<T...>;

    DataSource(const char *name) : name(name) {}
    virtual ~DataSource() {}

    virtual mp::optional<T...> get() = 0;
    const char *source_name() const {
        return name.data();
    }

private:
    std::string name;
};
#pragma once

#include <map>
#include <string>
#include <vector>
#include <functional>
#include "memory_view.hpp"
#include "http_encode.hpp"

template <typename T>
inline const char *unique_type_name();

#define REGISTER_UNIQUE_TYPE_NAME(T)                    \
        template<>                                      \
        inline const char *unique_type_name<T>() {      \
            return #T;                                  \
        }

class VectorPack {
    friend void from_json(const json &j, VectorPack &v) {
        j.at("data").get_to(v.data);
    }
    friend void to_json(json &j, const VectorPack &v) {
        j = json{{"data", v.data}};
    }

public:
    VectorPack() {}
    VectorPack(VectorPack &&) = default;

    template <typename... Ts>
    VectorPack(std::vector<Ts> &&...vectors) {
        add_helper(std::move(vectors)...);
    }

    template <typename T>
    inline bool contains() const {
        return data.find(unique_type_name<T>()) != data.end();
    }

    template <typename T>
    inline T &at(size_t i) & {
        auto iter = data.find(unique_type_name<T>());
        if (iter == data.end()) {
            throw std::runtime_error("no member of such type [" + std::string(unique_type_name<T>()) + "]");
        } else if (iter->second.size() / sizeof(T) < i + 1) {
            throw std::out_of_range("index out of range [" + std::to_string(i) + "]");
        }
        return *(reinterpret_cast<T*>(iter->second.data()) + i);
    }

    template <typename T>
    inline const T &at(size_t i) const & {
        auto iter = data.find(unique_type_name<T>());
        if (iter == data.end()) {
            throw std::runtime_error("no member of such type [" + std::string(unique_type_name<T>()) + "]");
        } else if (iter->second.size() / sizeof(T) < i + 1) {
            throw std::out_of_range("index out of range [" + std::to_string(i) + "]");
        }
        return *(reinterpret_cast<const T*>(iter->second.data()) + i);
    }

    template <typename T>
    inline size_t size() const {
        auto iter = data.find(unique_type_name<T>());
        if (iter == data.end()) {
            return 0;
        } else {
            return iter->second.size() / sizeof(T);
        }
    }

    template <typename T>
    inline void add(std::vector<T> &&vec) {
        data[unique_type_name<T>()] = Memory::SharedView<unsigned char>(std::move(vec));
        json_converter[unique_type_name<T>()] = [](const Memory::SharedView<unsigned char> &data, size_t index) -> json {
            if ((index + 1) * sizeof(T) > data.size()) {
                throw std::out_of_range("");
            }
            auto p = reinterpret_cast<const T*>(data.data());
            return json(p[index]);
        };
    }

    inline json get_json() const {
        json j = std::vector<json>();
        for (auto &pair : data) {
            auto &conv = json_converter.at(pair.first);
            for (size_t i = 0; ; i++) {
                json result;
                try {
                    result = conv(pair.second, i);
                } catch (std::out_of_range &e) {
                    break;
                }
                if (j.size() <= i) {
                    j.push_back(json());
                }
                j[i][pair.first] = result;
            }
        }

        return j;
    }

private:
    template <typename T, typename... Ts>
    inline void add_helper(std::vector<T> &&vec, std::vector<Ts> &&...rest) {
        add(std::move(vec));
        add_helper(std::move(rest)...);
    }

    inline void add_helper() {}

    std::map<std::string, Memory::SharedView<unsigned char>> data;
    std::map<std::string, std::function<json(const Memory::SharedView<unsigned char>, size_t)>> json_converter;
};

template <>
struct HttpEncode<VectorPack> {
public:
    static const char *mimetype() {
        return HttpEncode<json>::mimetype();
    }
    static Memory::SharedView<unsigned char> encode(const VectorPack &vp) {
        return HttpEncode<json>::encode(vp.get_json());
    }
};

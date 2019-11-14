#pragma once

#include <map>
#include <string>
#include <vector>
#include <functional>
#include "memory/bytes.hpp"
#include "logging.hpp"
#include "json.hpp"

template <typename T>
inline const char *unique_type_name() {
    return typeid(T).name();
}

#define REGISTER_UNIQUE_TYPE_NAME(T)                    \
        template<>                                      \
        inline const char *unique_type_name<T>() {      \
            return #T;                                  \
        }

class VectorPack {
public:
    class HolderBase {
    public:
        virtual ~HolderBase() {}
        virtual size_t size() const = 0;
        virtual bool json_convertible() const = 0;
        virtual json get_json(size_t index) const = 0;
    };

    template <typename T, bool ToJson>
    class HolderImpl;

    template <typename T>
    class HolderImpl<T, true> : public HolderBase {
    public:
        HolderImpl(std::vector<T> &&vec) : vec_(std::move(vec)) {}
        ~HolderImpl() {}

        size_t size() const override {
            return vec_.size();
        }

        bool json_convertible() const override {
            return true;
        }

        json get_json(size_t index) const override {
            if (index >= vec_.size()) {
                throw std::out_of_range("index out of range");
            }
            return json(vec_[index]);
        }

        std::vector<T> vec_;
    };

    template <typename T>
    class HolderImpl<T, false> : public HolderBase {
    public:
        HolderImpl(std::vector<T> &&vec) : vec_(std::move(vec)) {}
        ~HolderImpl() {}

        size_t size() const override {
            return vec_.size();
        }

        bool json_convertible() const override {
            return false;
        }

        json get_json(size_t index) const override {
            if (index >= vec_.size()) {
                throw std::out_of_range("index out of range");
            }

            throw std::runtime_error("failed converting");
        }

        std::vector<T> vec_;
    };

    template <typename T>
    using Holder = HolderImpl<T, std::is_convertible<json, const T &>::value>;

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
    inline std::vector<T> &get() {
        auto iter = data.find(unique_type_name<T>());
        if (iter == data.end()) {
            throw std::runtime_error("no member of such type [" + std::string(unique_type_name<T>()) + "]");
        }
        auto p = dynamic_cast<Holder<T>*>(iter->second.get());
        if (p == nullptr) {
            throw std::runtime_error("no member of such type [" + std::string(unique_type_name<T>()) + "]");
        }
        return p->vec_;
    }

    template <typename T>
    inline const std::vector<T> &get() const {
        auto iter = data.find(unique_type_name<T>());
        if (iter == data.end()) {
            throw std::runtime_error("no member of such type [" + std::string(unique_type_name<T>()) + "]");
        }
        auto p = dynamic_cast<const Holder<T>*>(iter->second.get());
        if (p == nullptr) {
            throw std::runtime_error("no member of such type [" + std::string(unique_type_name<T>()) + "]");
        }
        return p->vec_;
    }

    template <typename T>
    inline void add(std::vector<T> &&vec) {
        data[unique_type_name<T>()] = std::unique_ptr<Holder<T>>(new Holder<T>(std::move(vec)));
    }

    inline json get_json() const {
        json j = std::vector<json>();

        for (auto &pair : data) {
            if (!pair.second->json_convertible()) {
                continue;
            }

            for (size_t i = 0; i < pair.second->size(); i++) {
                if (j.size() <= i) {
                    j.push_back(json());
                }
                j[i][pair.first] = pair.second->get_json(i);;
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

    std::map<std::string, std::unique_ptr<HolderBase>> data;
};

#pragma once

#include <unordered_map>
#include <list>
#include "optional.hpp"

template <typename T>
class LRUCache {
public:
    LRUCache(size_t capacity) : capacity_(capacity) {}

    mp::optional<T> put(const T &value) {
        mp::optional<T> removed;

        if (values_.size() >= capacity_) {
            removed.emplace(values_.back());
            remove(values_.back());
        }
        auto iter = hash_.find(value);
        if (iter == hash_.end()) {
            values_.push_front(value);
            hash_[value] = values_.begin();
        } else {
            values_.erase(iter->second);
            values_.push_front(value);
            iter->second = values_.begin();
        }

        return removed;
    }

    bool contains(const T &value) {
        return (hash_.find(value) != hash_.end());
    }

    void remove(const T &value) {
        auto iter = hash_.find(value);
        values_.erase(iter->second);
        hash_.erase(iter);
    }

private:
    size_t capacity_;
    std::list<T> values_;
    std::unordered_map<T, typename decltype(values_)::iterator> hash_;
};

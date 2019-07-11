#pragma once

#include <vector>
#include <memory>
#include <utility>
#include <type_traits>
#include <cstring>
#include "range.hpp"
#include "data_convert.hpp"
#include "json.hpp"
#include "base64.hpp"

namespace Memory {
    struct internal_t {};

    class Holder {
    public:
        virtual ~Holder() {}
    };

    template <typename T>
    class HolderImpl : public Holder {
    public:
        template <typename U>
        HolderImpl(U &&obj) : object(std::forward<U>(obj)) {}
        ~HolderImpl() {}

        T object;
    };

    template <typename T>
    class SharedView {
        template <typename U>
        friend class SharedView;
        static_assert(std::is_pod<T>::value, "obj of Memory::SharedView must be POD");

        SharedView(internal_t, std::shared_ptr<Holder> holder, void *ptr, Range range) : holder_(holder), ptr_((unsigned char *)ptr), range_(range) {}
    public:
        SharedView() : ptr_(nullptr), range_({0, 0}) {}

        explicit SharedView(const T &obj) {
            auto tmp_holder = std::make_shared<HolderImpl<T>>(obj);
            ptr_ = (unsigned char*)&tmp_holder->object;
            range_ = {0, sizeof(T)};
            holder_ = tmp_holder;
        }

        template <typename U, typename = typename std::enable_if<std::is_rvalue_reference<U &&>::value>::type>
        SharedView(U &&obj, void *ptr, size_t begin, size_t end) : holder_(std::make_shared<HolderImpl<typename std::decay<U>::type>>(std::move(obj))), ptr_((unsigned char *)ptr), range_({begin, end}) {}

        template <typename U>
        SharedView(const SharedView<U> &other) : holder_(other.holder_), ptr_(other.ptr_), range_(other.range_) {}

        SharedView(std::string &&str) {
            auto tmp_holder = std::make_shared<HolderImpl<std::string>>(std::move(str));
            ptr_ = (unsigned char*)tmp_holder->object.data();
            range_ = {0, tmp_holder->object.length()};
            holder_ = tmp_holder;
        }

        template <typename U>
        SharedView(std::vector<U> &&vec) {
            auto tmp_holder = std::make_shared<HolderImpl<std::vector<U>>>(std::move(vec));
            ptr_ = (unsigned char*)tmp_holder->object.data();
            range_ = {0, sizeof(U) * tmp_holder->object.size()};
            holder_ = tmp_holder;
        }

        template <typename U, typename = typename std::enable_if<sizeof(U) % sizeof(T) == 0>::type>
        SharedView<U> slice_front(size_t count) {
            count = std::min<size_t>((range_.end - range_.begin) / sizeof(U), count);
            Range slice_range{range_.begin, range_.begin + count * sizeof(U)};
            range_.begin = slice_range.end;
            return SharedView<U>(internal_t(), holder_, ptr_, slice_range);
        }

        size_t size() const {
            return (range_.end - range_.begin) / sizeof(T);
        }
        T *data() {
            return reinterpret_cast<T*>(ptr_ + range_.begin);
        }
        T *begin() {
            return reinterpret_cast<T*>(ptr_ + range_.begin);
        }
        T *end() {
            return begin() + size();
        }
        const T *data() const {
            return reinterpret_cast<T*>(ptr_ + range_.begin);
        }
        const T *begin() const {
            return reinterpret_cast<T*>(ptr_ + range_.begin);
        }
        const T *end() const {
            return begin() + size();
        }
        T &operator[](size_t index) {
            return *(begin() + index);
        }
        const T &operator[](size_t index) const {
            return *(begin() + index);
        }

        /*
        Sets the range this memory view refers to.
        It does NOT allocate memory or grant access to this view.
        It is undefined behavior if the range given is not already accessible by the view.
        */
        void set_range(size_t begin, size_t end) {
            range_.begin = begin;
            range_.end = end;
        }

    private:
        std::shared_ptr<Holder> holder_;
        unsigned char *ptr_;
        Range range_;
    };

    inline void from_json(const json &j, SharedView<unsigned char> &v) {
        auto s = j.get<std::string>();
        v = base64::decode(s.begin(), s.end());
    }
    inline void to_json(json &j, const SharedView<unsigned char> &v) {
        j = base64::encode(v.begin(), v.end());
    }

    template <typename T>
    class View {
        template <typename U>
        friend class View;
        static_assert(std::is_pod<T>::value, "obj of View must be POD");
    public:
        View() : underlying_(), range_({0, 0}) {}
        View(const T &value) : underlying_(sizeof(T)), range_({0, sizeof(T)}) {
            memcpy(underlying_.data(), &value, sizeof(T));
        }
        View(std::vector<unsigned char> &&data) : underlying_(std::move(data)) {
            range_.begin = 0;
            range_.end = underlying_.size() - underlying_.size() % sizeof(T);
        }
        template <typename U>
        View(View<U> &&other) : underlying_(std::move(other.underlying_)) {
            range_.begin = other.range_.begin;
            size_t length = other.range_.end - other.range_.begin;
            range_.end = other.range_.begin + length - length % sizeof(T);
            other.range_ = Range{0, 0};
        }

        View &operator=(std::vector<unsigned char> &&data) {
            underlying_ = std::move(data);
            range_.begin = 0;
            range_.end = underlying_.size();
        }

        void push_back(const T &v) {
            resize(size() + 1);
            *(end() - 1) = v;
        }
        T &back() {
            return *(end() - 1);
        }
        T *data() {
            return begin();
        }
        T *begin() {
            return reinterpret_cast<T*>(underlying_.data() + range_.begin);
        }
        T *end() {
            return begin() + size();
        }
        const T *begin() const {
            return reinterpret_cast<T*>(underlying_.data() + range_.begin);
        }
        const T *end() const {
            return begin() + size();
        }
        T &operator[](size_t index) {
            return *(begin() + index);
        }
        const T &operator[](size_t index) const {
            return *(begin() + index);
        }
        size_t size() const {
            return (range_.end - range_.begin) / sizeof(T);
        }
        void resize(size_t size) {
            underlying_.resize(range_.begin + size * sizeof(T));
            range_.end = underlying_.size();
        }
        bool empty() const {
            return size() > 0;
        }
        Memory::SharedView<T> to_shared() {
            Memory::SharedView<T> shared(std::move(underlying_), underlying_.data(), range_.begin, range_.end);
            underlying_.clear();
            range_ = {0, 0};
            return shared;
        }

    private:
        std::vector<unsigned char> underlying_;
        Range range_;
    };
}

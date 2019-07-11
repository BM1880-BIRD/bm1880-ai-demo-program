#pragma once

#include <type_traits>
#include <vector>
#include <string>
#include <tuple>
#include "memory_view.hpp"

template <typename T>
class Encoding {
    static_assert(std::is_pod<T>::value, "encoding of non-POD types must be specialized");
public:
    template <typename Inserter>
    static void encode(const T &data, Inserter inserter) {
        Memory::View<T> view;
        view.push_back(data);
        inserter = view.to_shared();
    }

    template <typename Iterator>
    static Iterator decode(Iterator begin, Iterator end, T &output) {
        if (begin == end) {
            return begin;
        }
        memcpy(&output, begin->data(), sizeof(T));

        return std::next(begin);
    }
};

template <typename T, typename Inserter>
void encode(T &&data, Inserter inserter) {
    Encoding<typename std::decay<T>::type>::encode(std::forward<T>(data), inserter);
}

template <typename T, typename... Ts, typename Inserter>
void encode(T &&data, Ts &&...rest, Inserter inserter) {
    encode(std::forward<T>(data), inserter);
    encode(std::forward<Ts>(rest)..., inserter);
}

template <typename T, typename Iterator>
Iterator decode(Iterator begin, Iterator end, T &output) {
    return Encoding<typename std::decay<T>::type>::decode(begin, end, output);
}

template <>
class Encoding<std::string> {
public:
    template <typename Inserter>
    static void encode(const std::string &data, Inserter inserter) {
        inserter = Memory::SharedView<unsigned char>(std::vector<unsigned char>{data.begin(), data.end()});
    }

    template <typename Iterator>
    static Iterator decode(Iterator begin, Iterator end, std::string &output) {
        if (begin == end) {
            return begin;
        }
        output = std::string(begin->begin(), begin->end());
        return std::next(begin);
    }
};

template <typename T>
class Encoding<Memory::View<T>> {
public:
    template <typename Inserter>
    static void encode(Memory::View<T> &&data, Inserter inserter) {
        inserter = Memory::View<unsigned char>(std::move(data));
    }

    template <typename Iterator>
    static Iterator decode(Iterator begin, Iterator end, Memory::View<T> &output) {
        if (begin == end) {
            return begin;
        }
        output = std::move(*begin);
        return std::next(begin);
    }
};

template <typename T>
class Encoding<Memory::SharedView<T>> {
public:
    template <typename Inserter>
    static void encode(Memory::SharedView<T> &&data, Inserter inserter) {
        inserter = Memory::SharedView<unsigned char>(std::move(data));
    }

    template <typename Iterator>
    static Iterator decode(Iterator begin, Iterator end, Memory::SharedView<T> &output) {
        if (begin == end) {
            return begin;
        }
        output = std::move(*begin);
        return std::next(begin);
    }
};

template <size_t Index, typename... Ts>
class TupleEncoding {
public:
    template <typename Inserter>
    static void encode(std::tuple<Ts...> &&data, Inserter inserter) {
        TupleEncoding<Index - 1, Ts...>::encode(std::move(data), inserter);
        ::encode(std::move(std::get<Index - 1>(data)), inserter);
    }
    template <typename Iterator>
    static Iterator decode(Iterator begin, Iterator end, std::tuple<Ts...> &output) {
        if (begin == end) {
            return begin;
        }
        begin = TupleEncoding<Index - 1, Ts...>::decode(begin, end, output);
        return ::decode(begin, end, std::get<Index - 1>(output));
    }
};

template <typename... Ts>
class TupleEncoding<0, Ts...> {
public:
    template <typename Inserter>
    static void encode(std::tuple<Ts...> &&data, Inserter inserter) {
    }
    template <typename Iterator>
    static Iterator decode(Iterator begin, Iterator end, std::tuple<Ts...> &output) {
        return begin;
    }
};

template <typename... Ts>
class Encoding<std::tuple<Ts...>> {
public:

    template <typename Inserter>
    static void encode(std::tuple<Ts...> &&data, Inserter inserter) {
        TupleEncoding<sizeof...(Ts), Ts...>::encode(std::move(data), inserter);
    }

    template <typename Iterator>
    static Iterator decode(Iterator begin, Iterator end, std::tuple<Ts...> &output) {
        return TupleEncoding<sizeof...(Ts), Ts...>::decode(begin, end, output);
    }
};

template <typename T>
class Encoding<std::vector<T>> {
    static_assert(std::is_pod<T>::value, "element of encoded vector must be POD");

public:
    template <typename Inserter>
    static void encode(std::vector<T> &&data, Inserter inserter) {
        std::vector<unsigned char> buffer(sizeof(T) * data.size());
        memcpy(buffer.data(), data.data(), buffer.size());
        inserter = std::move(buffer);
    }

    template <typename Iterator>
    static Iterator decode(Iterator begin, Iterator end, std::vector<T> &output) {
        output.resize(begin->size() / sizeof(T));
        memcpy(output.data(), begin->data(), output.size() * sizeof(T));
        return begin + 1;
    }
};
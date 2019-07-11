#pragma once

#include <string>

template <char... Cs>
class TypeString {
};

template <>
class TypeString<> {
public:
    template <typename Iterator>
    static bool equals(Iterator begin, Iterator end) {
        return begin == end;
    }
};

template <char C, char... Cs>
class TypeString<C, Cs...> {
public:
    static bool equals(const std::string &s) {
        return equals(s.begin(), s.end());
    }
    template <typename Iterator>
    static bool equals(Iterator begin, Iterator end) {
        return (begin != end) && (*begin == C) && TypeString<Cs...>::equals(begin + 1, end);
    }
};

class Wildcard {
public:
    static bool equals(const std::string &) {
        return true;
    }
};

template <size_t N, size_t M>
constexpr char string_litera_at(char const(&c)[M]) noexcept {
    return N < M ? c[N] : '\0';
}

template <char A, char... X, char... Y>
decltype(concat(TypeString<X..., A>(), TypeString<Y>()...)) concat(TypeString<X...>, TypeString<A>, TypeString<Y>...);

template <char... X, char... Y>
TypeString<X...> concat(TypeString<X...>, TypeString<'\0'>, TypeString<Y>...);

template <char... X>
decltype(concat(TypeString<X>()...)) trim(TypeString<X...>);

#define TYPE_STRING_1(n, x)     string_litera_at<n>(x)
#define TYPE_STRING_2(n, x)     TYPE_STRING_1(n, x), TYPE_STRING_1(n + 1, x)
#define TYPE_STRING_4(n, x)     TYPE_STRING_2(n, x), TYPE_STRING_2(n + 2, x)
#define TYPE_STRING_8(n, x)     TYPE_STRING_4(n, x), TYPE_STRING_4(n + 4, x)
#define TYPE_STRING_16(n, x)    TYPE_STRING_8(n, x), TYPE_STRING_8(n + 8, x)
#define TYPE_STRING_32(n, x)    TYPE_STRING_16(n, x), TYPE_STRING_16(n + 16, x)

#define TYPE_STRING(x)          decltype(trim(TypeString<TYPE_STRING_32(0, x)>()))

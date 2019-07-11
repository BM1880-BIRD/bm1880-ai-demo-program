#pragma once

#include <cmath>

namespace face {

template <typename LIterator, typename RIterator>
double compute_score(LIterator lhs_begin, LIterator lhs_end, RIterator rhs_begin, RIterator rhs_end) {
    double lhs_sq_mag = 0;
    double rhs_sq_mag = 0;
    double cdot = 0;
    for (; lhs_begin != lhs_end && rhs_begin != rhs_end; lhs_begin++, rhs_begin++) {
        auto lhs = *lhs_begin;
        auto rhs = *rhs_begin;
        cdot += lhs * rhs;
        lhs_sq_mag += lhs * lhs;
        rhs_sq_mag += rhs * rhs;
    }
    if (lhs_sq_mag == 0 || rhs_sq_mag == 0) {
        return 0;
    }

    return cdot / sqrt(lhs_sq_mag) / sqrt(rhs_sq_mag);
}

}

#pragma once

#include <list>
#include <algorithm>

namespace math {

struct Range {
    size_t begin;
    size_t end;
};

struct RangeSet {
    std::list<Range> ranges;

    size_t length() const {
        size_t sum = 0;
        for (auto &range : ranges) {
            sum += range.end - range.begin;
        }
        return sum;
    }

    bool empty() const {
        return !std::any_of(ranges.begin(), ranges.end(), [](const Range &r) {
            return r.end > r.begin;
        });
    }

    bool contains(size_t value) const {
        return std::any_of(ranges.begin(), ranges.end(), [value](const Range &r) {
            return r.begin <= value && r.end > value;
        });
    }

    size_t front() const {
        for (auto &range : ranges) {
            if (range.begin < range.end) {
                return range.begin;
            }
        }

        return 0;
    }

    void remove(size_t value) {
        (*this) -= Range{value, value + 1};
    }

    RangeSet operator&(const Range &rhs) const {
        RangeSet intersection;
        for (auto &lhs : ranges) {
            if (lhs.begin >= rhs.end) {
                break;
            } else if (lhs.end <= rhs.begin) {
                continue;
            } else {
                Range s;
                s.begin = std::max(lhs.begin, rhs.begin);
                s.end = std::min(lhs.end, rhs.end);
                if (s.begin < s.end) {
                    intersection.ranges.push_back(s);
                }
            }
        }
        return intersection;
    }

    RangeSet &operator-=(const Range &range) {
        for (auto it = ranges.begin(); it != ranges.end();) {
            if (it->begin >= range.end) {
                break;
            } else if (it->end <= range.begin) {
                it++;
            } else {
                if ((it->begin < range.begin) && (it->end > range.end)) {
                    ranges.insert(it, {it->begin, range.begin});
                    it->begin = range.end;
                } else if (it->begin >= range.begin) {
                    it->begin = range.end;
                } else {
                    it->end = range.begin;
                }

                if (it->end <= it->begin) {
                    it = ranges.erase(it);
                } else {
                    it++;
                }
            }
        }
        return *this;
    }
};

}
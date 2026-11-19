/* This file is part of the dynarmic project.
 * Copyright (c) 2026 Superhac
 * SPDX-License-Identifier: 0BSD
 */

#pragma once

#include <iterator>
#include <limits>
#include <map>
#include <type_traits>

namespace Dynarmic::Common {

/// A closed interval [lower, upper] of an unsigned integer type.
template<typename T>
struct ClosedInterval {
    T lower;
    T upper;
};

/// A set of integers, kept as disjoint, non-adjacent closed intervals ordered by their lower bound.
/// Adding an interval merges it with every interval it overlaps or touches.
template<typename T>
class IntervalSet {
    static_assert(std::is_unsigned_v<T>);
    using Map = std::map<T, T>;  // lower -> upper

public:
    class const_iterator {
    public:
        using iterator_category = std::bidirectional_iterator_tag;
        using value_type = ClosedInterval<T>;
        using difference_type = std::ptrdiff_t;
        using pointer = void;
        using reference = value_type;

        const_iterator() = default;
        explicit const_iterator(typename Map::const_iterator it)
                : it(it) {}

        value_type operator*() const { return {it->first, it->second}; }
        const_iterator& operator++() {
            ++it;
            return *this;
        }
        const_iterator operator++(int) { return const_iterator{it++}; }
        const_iterator& operator--() {
            --it;
            return *this;
        }
        const_iterator operator--(int) { return const_iterator{it--}; }
        bool operator==(const const_iterator& other) const { return it == other.it; }
        bool operator!=(const const_iterator& other) const { return it != other.it; }

    private:
        typename Map::const_iterator it;
    };

    /// Adds [lower, upper]. An empty interval (upper < lower) is ignored.
    void add(T lower, T upper) {
        if (upper < lower) {
            return;
        }

        // The first interval that could overlap or touch: the last one starting at or before lower.
        auto it = intervals.upper_bound(lower);
        if (it != intervals.begin()) {
            auto prev = std::prev(it);
            if (prev->second == std::numeric_limits<T>::max() || prev->second + 1 >= lower) {
                it = prev;
            }
        }

        // Absorb every interval that starts at or before upper + 1.
        while (it != intervals.end() && (upper == std::numeric_limits<T>::max() || it->first <= upper + 1)) {
            if (it->first < lower) {
                lower = it->first;
            }
            if (it->second > upper) {
                upper = it->second;
            }
            it = intervals.erase(it);
        }

        intervals.emplace_hint(it, lower, upper);
    }

    void clear() { intervals.clear(); }
    bool empty() const { return intervals.empty(); }
    std::size_t size() const { return intervals.size(); }  ///< Number of disjoint intervals.

    const_iterator begin() const { return const_iterator{intervals.begin()}; }
    const_iterator end() const { return const_iterator{intervals.end()}; }

private:
    Map intervals;
};

}  // namespace Dynarmic::Common

/* This file is part of the dynarmic project.
 * Copyright (c) 2026 Superhac
 * SPDX-License-Identifier: 0BSD
 */

#include <algorithm>
#include <limits>
#include <utility>
#include <vector>

#include <catch2/catch_test_macros.hpp>
#include <mcl/stdint.hpp>

#include "dynarmic/backend/block_range_information.h"
#include "dynarmic/common/interval_set.h"

using namespace Dynarmic;

namespace {

template<typename T>
std::vector<std::pair<T, T>> Intervals(const Common::IntervalSet<T>& set) {
    std::vector<std::pair<T, T>> result;
    for (const auto interval : set) {
        result.emplace_back(interval.lower, interval.upper);
    }
    return result;
}

std::vector<u64> Sorted(const tsl::robin_set<IR::LocationDescriptor>& locations) {
    std::vector<u64> result;
    for (const auto& location : locations) {
        result.push_back(location.Value());
    }
    std::sort(result.begin(), result.end());
    return result;
}

}  // namespace

TEST_CASE("IntervalSet: disjoint intervals stay apart, in order", "[interval_set]") {
    Common::IntervalSet<u32> set;
    REQUIRE(set.empty());
    set.add(20, 29);
    set.add(0, 9);
    REQUIRE(Intervals(set) == std::vector<std::pair<u32, u32>>{{0, 9}, {20, 29}});
    set.clear();
    REQUIRE(set.empty());
}

TEST_CASE("IntervalSet: overlapping and adjacent intervals merge", "[interval_set]") {
    Common::IntervalSet<u32> set;
    set.add(10, 19);
    set.add(15, 25);  // overlaps
    REQUIRE(Intervals(set) == std::vector<std::pair<u32, u32>>{{10, 25}});
    set.add(26, 30);  // adjacent above
    set.add(5, 9);    // adjacent below
    REQUIRE(Intervals(set) == std::vector<std::pair<u32, u32>>{{5, 30}});
    set.add(40, 50);
    set.add(60, 70);
    set.add(0, 100);  // swallows everything
    REQUIRE(Intervals(set) == std::vector<std::pair<u32, u32>>{{0, 100}});
}

TEST_CASE("IntervalSet: an interval bridging two others joins them", "[interval_set]") {
    Common::IntervalSet<u32> set;
    set.add(0, 9);
    set.add(20, 29);
    set.add(10, 19);
    REQUIRE(Intervals(set) == std::vector<std::pair<u32, u32>>{{0, 29}});
}

TEST_CASE("IntervalSet: empty intervals are ignored", "[interval_set]") {
    Common::IntervalSet<u32> set;
    set.add(10, 9);
    REQUIRE(set.empty());
}

TEST_CASE("IntervalSet: the ends of the type", "[interval_set]") {
    constexpr u32 max = std::numeric_limits<u32>::max();
    Common::IntervalSet<u32> set;
    set.add(max - 9, max);
    set.add(0, 0);
    REQUIRE(Intervals(set) == std::vector<std::pair<u32, u32>>{{0, 0}, {max - 9, max}});
    set.add(max - 20, max - 10);  // adjacent below an interval ending at max
    REQUIRE(Intervals(set) == std::vector<std::pair<u32, u32>>{{0, 0}, {max - 20, max}});
    set.add(1, max - 21);  // fills the gap
    REQUIRE(Intervals(set) == std::vector<std::pair<u32, u32>>{{0, max}});

    Common::IntervalSet<u64> set64;
    set64.add(0, std::numeric_limits<u64>::max());
    set64.add(5, 6);
    REQUIRE(Intervals(set64) == std::vector<std::pair<u64, u64>>{{0, std::numeric_limits<u64>::max()}});
}

TEST_CASE("BlockRangeInformation: finds the blocks overlapping an invalidated range", "[interval_set]") {
    Backend::BlockRangeInformation<u32> blocks;
    blocks.AddRange(0x1000, 0x100F, IR::LocationDescriptor{1});
    blocks.AddRange(0x1010, 0x103F, IR::LocationDescriptor{2});
    blocks.AddRange(0x2000, 0x2003, IR::LocationDescriptor{3});
    blocks.AddRange(0x0800, 0x2FFF, IR::LocationDescriptor{4});  // a long block covering the rest

    Common::IntervalSet<u32> ranges;
    ranges.add(0x100F, 0x1010);  // the last byte of 1 and the first of 2
    REQUIRE(Sorted(blocks.InvalidateRanges(ranges)) == std::vector<u64>{1, 2, 4});

    ranges.clear();
    ranges.add(0x3000, 0x3FFF);  // just past the end of every block
    REQUIRE(blocks.InvalidateRanges(ranges).empty());

    ranges.clear();
    ranges.add(0x1040, 0x1FFF);  // between 2 and 3: only the long block
    REQUIRE(Sorted(blocks.InvalidateRanges(ranges)) == std::vector<u64>{4});

    ranges.add(0x2003, 0x2003);
    REQUIRE(Sorted(blocks.InvalidateRanges(ranges)) == std::vector<u64>{3, 4});
}

TEST_CASE("BlockRangeInformation: re-registered blocks, empty ranges and clearing", "[interval_set]") {
    Backend::BlockRangeInformation<u64> blocks;
    blocks.AddRange(100, 199, IR::LocationDescriptor{7});
    blocks.AddRange(100, 199, IR::LocationDescriptor{7});  // the same block, translated again
    blocks.AddRange(300, 299, IR::LocationDescriptor{8});  // an empty block covers nothing

    Common::IntervalSet<u64> ranges;
    ranges.add(0, 1000);
    REQUIRE(Sorted(blocks.InvalidateRanges(ranges)) == std::vector<u64>{7});

    blocks.ClearCache();
    REQUIRE(blocks.InvalidateRanges(ranges).empty());
}

TEST_CASE("BlockRangeInformation: blocks at the top of the address space", "[interval_set]") {
    constexpr u32 max = std::numeric_limits<u32>::max();
    Backend::BlockRangeInformation<u32> blocks;
    blocks.AddRange(max - 3, max, IR::LocationDescriptor{1});
    blocks.AddRange(0, 3, IR::LocationDescriptor{2});

    Common::IntervalSet<u32> ranges;
    ranges.add(max, max);
    REQUIRE(Sorted(blocks.InvalidateRanges(ranges)) == std::vector<u64>{1});

    ranges.clear();
    ranges.add(0, max);  // the whole address space
    REQUIRE(Sorted(blocks.InvalidateRanges(ranges)) == std::vector<u64>{1, 2});
}

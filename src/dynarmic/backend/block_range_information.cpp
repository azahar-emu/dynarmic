/* This file is part of the dynarmic project.
 * Copyright (c) 2018 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include "dynarmic/backend/block_range_information.h"

#include <mcl/stdint.hpp>
#include <tsl/robin_set.h>

namespace Dynarmic::Backend {

template<typename ProgramCounterType>
void BlockRangeInformation<ProgramCounterType>::AddRange(ProgramCounterType first, ProgramCounterType last, IR::LocationDescriptor location) {
    if (last < first) {
        return;
    }
    block_ranges[first].emplace(last, location);
    if (last - first > max_extent) {
        max_extent = last - first;
    }
}

template<typename ProgramCounterType>
void BlockRangeInformation<ProgramCounterType>::ClearCache() {
    block_ranges.clear();
    max_extent = 0;
}

template<typename ProgramCounterType>
tsl::robin_set<IR::LocationDescriptor> BlockRangeInformation<ProgramCounterType>::InvalidateRanges(const Common::IntervalSet<ProgramCounterType>& ranges) {
    tsl::robin_set<IR::LocationDescriptor> erase_locations;
    for (const auto invalidate_interval : ranges) {
        const ProgramCounterType lower = invalidate_interval.lower;
        const ProgramCounterType upper = invalidate_interval.upper;
        // A block [first, last] overlaps [lower, upper] when first <= upper and last >= lower;
        // last - first <= max_extent, so first >= lower - max_extent.
        const ProgramCounterType search_from = lower > max_extent ? lower - max_extent : 0;
        for (auto it = block_ranges.lower_bound(search_from); it != block_ranges.end() && it->first <= upper; ++it) {
            for (const auto& [last, descriptor] : it->second) {
                if (last >= lower) {
                    erase_locations.insert(descriptor);
                }
            }
        }
    }
    // TODO: EFFICIENCY: Remove ranges that are to be erased.
    return erase_locations;
}

template class BlockRangeInformation<u32>;
template class BlockRangeInformation<u64>;

}  // namespace Dynarmic::Backend

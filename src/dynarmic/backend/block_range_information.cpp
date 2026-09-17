/* This file is part of the dynarmic project.
 * Copyright (c) 2018 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include "dynarmic/backend/block_range_information.h"

#include <boost/icl/interval_map.hpp>
#include <boost/icl/interval_set.hpp>
#include <mcl/stdint.hpp>
#include <tsl/robin_set.h>

namespace Dynarmic::Backend {

template<typename ProgramCounterType>
void BlockRangeInformation<ProgramCounterType>::AddRange(boost::icl::discrete_interval<ProgramCounterType> range, IR::LocationDescriptor location) {
    block_ranges.add(std::make_pair(range, std::set<IR::LocationDescriptor>{location}));
}

template<typename ProgramCounterType>
void BlockRangeInformation<ProgramCounterType>::ClearCache() {
    block_ranges.clear();
}

template<typename ProgramCounterType>
tsl::robin_set<IR::LocationDescriptor> BlockRangeInformation<ProgramCounterType>::InvalidateRanges(const boost::icl::interval_set<ProgramCounterType>& ranges) {
    tsl::robin_set<IR::LocationDescriptor> erase_locations;
    for (auto invalidate_interval : ranges) {
        if (boost::icl::is_empty(invalidate_interval)) {
            continue;
        }
        // equal_range orders intervals with an exclusive-less comparison, and that
        // comparison is not a strict weak ordering once intervals overlap: every block
        // inside the invalidated range compares equivalent to the range while the blocks
        // stay ordered among themselves, so the search behind equal_range returns the
        // first block and stops. Walk the map instead, from the first block that can
        // reach into the range up to the first one that starts past it.
        const auto range_first = boost::icl::first(invalidate_interval);
        const auto range_last = boost::icl::last(invalidate_interval);
        const auto start = boost::icl::discrete_interval<ProgramCounterType>::closed(range_first, range_first);
        for (auto it = block_ranges.lower_bound(start); it != block_ranges.end(); ++it) {
            if (boost::icl::first(it->first) > range_last) {
                break;
            }
            for (const auto& descriptor : it->second) {
                erase_locations.insert(descriptor);
            }
        }
    }
    // TODO: EFFICIENCY: Remove ranges that are to be erased.
    return erase_locations;
}

template class BlockRangeInformation<u32>;
template class BlockRangeInformation<u64>;

}  // namespace Dynarmic::Backend

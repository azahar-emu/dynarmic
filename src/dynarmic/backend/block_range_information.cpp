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
        // equal_range cannot be used here: every block inside the invalidated
        // interval compares equivalent to it while the blocks stay ordered
        // among themselves, which is not a strict weak ordering, so it stops
        // after the first of them. Walk from the first block that reaches the
        // interval to the last one that starts inside it.
        const auto first = boost::icl::first(invalidate_interval);
        const auto last = boost::icl::last(invalidate_interval);
        for (auto it = block_ranges.lower_bound(boost::icl::discrete_interval<ProgramCounterType>::closed(first, first));
             it != block_ranges.end() && boost::icl::first(it->first) <= last;
             ++it) {
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

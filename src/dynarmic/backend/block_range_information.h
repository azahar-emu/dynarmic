/* This file is part of the dynarmic project.
 * Copyright (c) 2018 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#pragma once

#include <map>
#include <set>
#include <utility>

#include <tsl/robin_set.h>

#include "dynarmic/common/interval_set.h"
#include "dynarmic/ir/location_descriptor.h"

namespace Dynarmic::Backend {

template<typename ProgramCounterType>
class BlockRangeInformation {
public:
    /// Records that the block at `location` was translated from the guest addresses [first, last].
    void AddRange(ProgramCounterType first, ProgramCounterType last, IR::LocationDescriptor location);
    void ClearCache();
    /// Returns every recorded block whose range overlaps one of `ranges`.
    tsl::robin_set<IR::LocationDescriptor> InvalidateRanges(const Common::IntervalSet<ProgramCounterType>& ranges);

private:
    /// first address -> {last address, block}
    std::map<ProgramCounterType, std::set<std::pair<ProgramCounterType, IR::LocationDescriptor>>> block_ranges;
    /// The largest (last - first) recorded: no range starting more than this below an address can reach it.
    ProgramCounterType max_extent = 0;
};

}  // namespace Dynarmic::Backend

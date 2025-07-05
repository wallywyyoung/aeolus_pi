//
// Created by Wally Young on 5/31/25.
//

#include "aeolus/utilities/Range.h"

#include <algorithm>

Range::Range(const int start, const int end) : start(start), end(end) { }

bool Range::contains(const int value) const {
    return value >= start && value <= end;
}

Range Range::getUnionWith(const Range compare) {
    return Range(std::min(start, compare.start),std::max(end, compare.end));
}

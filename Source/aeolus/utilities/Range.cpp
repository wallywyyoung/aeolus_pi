//
// Created by Wally Young on 5/31/25.
//

#include "aeolus/utilities/Range.h"

#include <algorithm>

Range::Range(int start, int end) : start(start), end(end) { }

bool Range::contains(int value) const {
    return value >= start && value <= end;
}

Range Range::getUnionWith(Range compare) {
    return Range(std::min(start, compare.start),std::max(end, compare.end));
}

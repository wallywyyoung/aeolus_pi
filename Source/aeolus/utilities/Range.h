//
// Created by Wally Young on 5/31/25.
//

#ifndef AEOLUS_PI_RANGE_H
#define AEOLUS_PI_RANGE_H

//#include <set>
#include <algorithm>

class Range {
public:
    Range() = default;
    Range(int start, int end);
    bool contains(int value) const;
    Range getUnionWith(Range compare);
    int getStart() const { return start; }
    int getEnd() const { return end; }
private:
    int start, end;
};


#endif //AEOLUS_PI_RANGE_H

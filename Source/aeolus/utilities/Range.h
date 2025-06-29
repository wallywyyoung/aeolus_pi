//
// Created by Wally Young on 5/31/25.
//

#pragma once

class Range {
public:
    Range() = default;
    Range(int start, int end);
    [[nodiscard]] bool contains(int value) const;
    Range getUnionWith(Range compare);
    [[nodiscard]] int getStart() const { return start; }
    [[nodiscard]] int getEnd() const { return end; }
private:
    int start, end;
};

//
// Created by Wally Young on 6/27/25.
//

#include "aeolus/globals.h"

#include <array>
#include <nlohmann/json.hpp>



/**
 * @brief Interpolated per-note look-up table.
 *
 * This class stores a float parameter across the
 * N_NOTES points. Notes in between get interpolated linearly.
 */

class N_func final
{
public:
    N_func();
    void reset(float v);
    void setValue(int idx, float v);    // setv(i, v)
    void clearValue(int idx);           // clrv(i)
    float getValue(int idx) const;      // vs(i)
    bool isSet(int idx) const;          // st(i)

    /// Returns interpolated value for a note number (starting from 0).
    float operator[](int note) const;   // vi(n)

//    std::map<std::string, std::any> toVar() const;
    void fromJson(const nlohmann::json& v);
    void read(std::istream& stream);

private:
    int _b;
    std::array<float, N_NOTES> _v;
};



//
// Created by Wally Young on 6/27/25.
//

#include "HN_func.h"

using namespace aeolus;

HN_func::HN_func()
        : _h{}
{
}

void HN_func::reset(float v)
{
    for (auto& h : _h) {
        h.reset(v);
    }
}

void HN_func::setValue(int idx, float v)
{
    assert(isPositiveAndBelow(idx, N_NOTES));

    for (auto& h: _h) {
        h.setValue(idx, v);
    }
}

void HN_func::setValue(int harm, int idx, float v)
{
    assert(isPositiveAndBelow(harm, _h.size()));
    assert(isPositiveAndBelow(idx, N_NOTES));

    _h[harm].setValue(idx, v);
}

void HN_func::clearValue(int idx)
{
    assert(isPositiveAndBelow(idx, N_NOTES));

    for (auto& h : _h) {
        h.clearValue(idx);
    }
}

void HN_func::clearValue(int harm, int idx)
{
    assert(isPositiveAndBelow(harm, _h.size()));
    assert(isPositiveAndBelow(idx, N_NOTES));

    _h[harm].clearValue(idx);
}

float HN_func::getValue(int harm, int idx) const
{
    assert(isPositiveAndBelow(harm, _h.size()));
    assert(isPositiveAndBelow(idx, N_NOTES));

    return _h[harm].getValue(idx);
}

bool HN_func::isSet(int harm, int idx) const
{
    assert(isPositiveAndBelow(harm, _h.size()));
    assert(isPositiveAndBelow(idx, N_NOTES));

    return _h[harm].isSet(idx);
}

void HN_func::fromJson(const nlohmann::json& v)
{
    if (v.size() >= _h.size()) {
        for (int i = 0; i < _h.size(); ++i)
            _h[i].fromJson(v[i]);
    }
}

void HN_func::read(std::istream& stream, int n)
{
    const auto m = std::min(_h.size(), (size_t)n);

    for (int i = 0; i < m; ++i)
        _h[i].read(stream);
}

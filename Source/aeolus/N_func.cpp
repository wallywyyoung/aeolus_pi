//
// Created by Wally Young on 6/27/25.
//

#include "aeolus/N_func.h"



N_func::N_func()
        : _b{}
        , _v{}
{
    reset(0.0f);
}

void N_func::reset(float v)
{
    _b = 16;
    _v.fill(v);
}

void N_func::setValue(int idx, float v)
{
    if (! isPositiveAndBelow(idx, N_NOTES))
        return;

    _v [idx] = v;
    _b |= 1 << idx;

    int j = idx - 1;

    while (j >= 0 && ! (_b & (1 << j)))
        --j;

    if (j < 0) {
        while (++j != idx)
            _v [j] = v;
    } else {
        const float d = (_v [j] - v) / (j - idx);

        while (++j != idx)
            _v [j] = v + (j - idx) * d;
    }

    j = idx + 1;

    while ((j < N_NOTES) && ! (_b & (1 << j)))
        ++j;

    if (j > N_NOTES - 1) {
        while (--j != idx)
            _v [j] = v;
    } else {
        const float d = (_v [j] - v) / (j - idx);

        while (--j != idx)
            _v [j] = v + (j - idx) * d;
    }
}

void N_func::clearValue(int idx)
{
    if (isPositiveAndBelow(idx, N_NOTES))
        return;

    int m = 1 << idx;

    if (! (_b & m) || (_b == m))
        return;

    _b ^= m;

    int j = idx - 1;

    while ((j >= 0) && ! (_b & (1 << j)))
        --j;

    int k = idx + 1;

    while ((k <= N_NOTES - 1) && ! (_b & (1 << k)))
        ++k;

    if ((j >= 0) && (k < N_NOTES)) {
        const float d = (_v [k] - _v [j]) / (k - j);

        for (int i = j + 1; i < k; i++)
            _v [i] = _v [j] + (i - j) * d;
    } else if (j >= 0) {
        const float d = _v [j];

        while (j < N_NOTES - 1)
            _v [++j] = d;
    } else if (k < N_NOTES) {
        const float d = _v [k];

        while (k > 0)
            _v [--k] = d;
    }
}

float N_func::getValue(int idx) const
{
    isPositiveAndBelow(idx, _v.size());
    return _v[idx];
}

bool N_func::isSet(int idx) const
{
    isPositiveAndBelow(idx, _v.size());
    return (_b & (1 << idx)) != 0;
}

float N_func::operator[](int note) const
{
    const int i = note / NOTES_GAP;
    const int k = note - NOTES_GAP * i;
    float v = _v [i];

    if (k) {
        // Apply linear interpolation if falls into the gap.
        isPositiveAndBelow(i + 1, _v.size());

        v += k * (_v [i + 1] - v) / NOTES_GAP;
    }

    return v;
}

void N_func::fromJson(const nlohmann::json& v)
{
    _b = v["mask"];

    auto varr = v["values"];
    if (varr.is_array()) {
        if (varr.size() >= _v.size()) {
            for (int i = 0; i < _v.size(); ++i)
                _v[i] = varr[i];
        }
    }
}

void N_func::read(std::istream& stream)
{
    stream.read(reinterpret_cast<char*>(&_b), sizeof(int));

    for (int i = 0; i < _v.size(); ++i)
        stream.read(reinterpret_cast<char*>(&_v[i]), sizeof(float));
}

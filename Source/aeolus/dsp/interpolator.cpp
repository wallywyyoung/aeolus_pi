// ----------------------------------------------------------------------------
//
//  Copyright (C) 2021 Arthur Benilov <arthur.benilov@gmail.com>
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 3 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
// ----------------------------------------------------------------------------

#include "aeolus/dsp/interpolator.h"
#include "aeolus/globals.h"

#include <cstring>
#include <stdexcept>


namespace dsp {

Interpolator::Interpolator(float ratio, size_t nChannels)
    : _acc(nChannels)
    , _accIndex{0}
    , _accFrac{0.0f}
    , _ratio{ratio}
{
    if (nChannels <= 0) {
        throw std::invalid_argument("Interpolator: number of channels must be greater than zero");
    }
}

void Interpolator::setNumberOfChannels(size_t n)
{
    if (n <= 0) {
        throw std::invalid_argument("Interpolator: number of channels must be greater than zero");
    }
    _acc.resize(n);
    reset();
}

void Interpolator::reset()
{
    for (auto& buf : _acc) {
        ::memset(buf.data(), 0, sizeof(float) * 8);
    }

    _accIndex = 0;
    _accFrac = 0.0f;
}

bool Interpolator::canRead() const noexcept
{
    return _accFrac < 1.0f;
}

bool Interpolator::readAllChannels(float* const x) noexcept
{
    if (x == nullptr) {
        throw std::invalid_argument("Interpolator: x cannot be null");
    }

    if (_accFrac >= 1.0f)
        return false;

    for (size_t i = 0; i < _acc.size(); ++i) {
        x[i] = math::lagr(&_acc[i].data()[_accIndex], _accFrac);
    }

    _accFrac += _ratio;

    return true;
}

float Interpolator::readUnchecked(size_t channel) const noexcept
{
    if (_accFrac >= 1.0f) {
        throw std::invalid_argument("Interpolator: _accFrac must be less than 1.0");
    }

    return math::lagr(&_acc[channel].data()[_accIndex], _accFrac);
}

float Interpolator::readLinearUnchecked(size_t channel) const noexcept
{
    if (_accFrac >= 1.0f) {
        throw std::invalid_argument("Interpolator: _accFrac must be less than 1.0");
    }

    return math::lerp(_acc[channel].data()[_accIndex], _acc[channel].data()[_accIndex + 1], _accFrac);
}

void Interpolator::readIncrement()
{
    _accFrac += _ratio;
}

bool Interpolator::read(float& l, float& r) noexcept
{
    if (_acc.size() <= 1) {
        throw std::invalid_argument("Interpolator: number of channels must be greater than one");
    }

    if (_accFrac >= 1.0f)
        return false;

    l = math::lagr(&_acc[0].data()[_accIndex], _accFrac);
    r = math::lagr(&_acc[1].data()[_accIndex], _accFrac);

    _accFrac += _ratio;

    return true;
}

bool Interpolator::canWrite() const noexcept
{
    return _accFrac >= 1.0f;
}

bool Interpolator::writeAllChannels(const float* const x) noexcept
{
    if (x == nullptr) {
        throw std::invalid_argument("Interpolator: x cannot be null");
    }

    if (_accFrac < 1.0f)
        return false;

    for (size_t i = 0; i < _acc.size(); ++i) {
        _acc[i][_accIndex] = _acc[i][_accIndex + 4] = x[i];
    }

    _accIndex = (_accIndex + 1) % 4;
    _accFrac -= 1.0f;

    return true;
}

void Interpolator::writeUnchecked(float x, size_t channel)
{
    if (_acc.size() <= 1) {
        throw std::invalid_argument("Interpolator: number of channels must be greater than one");
    }

    _acc[channel][_accIndex] = _acc[channel][_accIndex + 4] = x;
}

void Interpolator::writeIncrement()
{
    _accIndex = (_accIndex + 1) % 4;
    _accFrac -= 1.0f;
}

bool Interpolator::write(float l, float r) noexcept
{
    if (_acc.size() <= 1) {
        throw std::invalid_argument("Interpolator: number of channels must be greater than one");
    }

    if (_accFrac < 1.0f)
        return false;

    _acc[0][_accIndex] = _acc[0][_accIndex + 4] = l;
    _acc[1][_accIndex] = _acc[1][_accIndex + 4] = r;

    _accIndex = (_accIndex + 1) % 4;
    _accFrac -= 1.0f;

    return true;
}

} // namespace dsp



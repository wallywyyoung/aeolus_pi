// ----------------------------------------------------------------------------
//
//  Copyright (C) 2025 Wally Young <wallywyyoung@users.noreply.github.com>
//  Copyright (C) 2021 Arthur Benilov <arthur.benilov@gmail.com>
//  Copyright (C) 2003-2013 Fons Adriaensen <fons@linuxaudio.org>
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

#include "aeolus/audioparam.h"
#include "aeolus/globals.h"

AudioParameter::AudioParameter(const float value, const float min, const float max, const float smooth) : _currentValue{value}, _minValue{min}, _maxValue{max}, _targetValue{value}, _frac{smooth}, _smoothing{false} { }

void AudioParameter::setValue(const float v, const float s, const bool force)
{
    _targetValue = limitRange(_minValue, _maxValue, v);

    _frac = limitRange(0.0f, 1.0f, s);

    if (force) {
        _currentValue = _targetValue;
        _smoothing = false;
    } else {
        updateSmoothing();
    }
}

void AudioParameter::setValue(const float v, const bool force)
{
    _targetValue = limitRange(_minValue, _maxValue, v);

    if (force) {
        _currentValue = _targetValue;
        _smoothing = false;
    } else {
        updateSmoothing();
    }
}

void AudioParameter::setSmoothing(const float s) noexcept
{
    _frac = limitRange(0.0f, 1.0f, s);
}

void AudioParameter::setRange(const float min, const float max)
{
    _minValue = std::min<float>(min, max);
    _maxValue = std::max<float>(min, max);
}

AudioParameter& AudioParameter::operator = (const float v)
{
    setValue(v);

    return *this;
}

float AudioParameter::nextValue()
{
    updateSmoothing();

    if (_smoothing) {
        const float prevValue { _currentValue };
        _currentValue = _targetValue * _frac + _currentValue * (1.0f - _frac);

        if (fabsf(_currentValue - prevValue) <= std::numeric_limits<float>::epsilon()) {
            // No advancement - jump to the target
            _currentValue = _targetValue;
        }
    }

    return _currentValue;
}

void AudioParameter::updateSmoothing()
{
    _smoothing = std::abs(_currentValue - _targetValue) > std::numeric_limits<float>::epsilon();

    if (!_smoothing)
        _currentValue = _targetValue;
}

//----------------------------------------------------------

AudioParameterPool::AudioParameterPool (const size_t size) : _params (size) { }

AudioParameter& AudioParameterPool::operator[] (const int index)
{
    if (index >= 0 && index < static_cast<int>(_params.size()))
        return _params.at(index);

    return _dummyParameter;
}

const AudioParameter& AudioParameterPool::operator[] (const int index) const
{
    if (index >= 0 && index < static_cast<int>(_params.size()))
        return _params.at(index);

    return _dummyParameter;
}

AudioParameter& AudioParameterPool::findByName(const std::string& n)
{
    for (auto& p : _params) {
        if (p.name() == n)
            return p;
    }

    return _dummyParameter;
}



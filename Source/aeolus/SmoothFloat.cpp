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

#include "aeolus/SmoothFloat.h"
#include "aeolus/globals.h"

#include <cmath>

SmoothFloat::SmoothFloat(const float value, const float min, const float max, const float smooth) : currentValue{value}, minValue{min}, maxValue{max}, targetValue{value}, frac{smooth}, smoothing{false} { }

void SmoothFloat::setValue(const float v, const float s, const bool force) {
    targetValue = limitRange(minValue, maxValue, v);
    frac = limitRange(0.0f, 1.0f, s);
    if (force) {
        currentValue = targetValue;
        smoothing = false;
    } else {
        updateSmoothing();
    }
}

void SmoothFloat::setValue(const float v, const bool force) {
    targetValue = limitRange(minValue, maxValue, v);
    if (force) {
        currentValue = targetValue;
        smoothing = false;
    } else {
        updateSmoothing();
    }
}

void SmoothFloat::setSmoothing(const float s) noexcept {
    frac = limitRange(0.0f, 1.0f, s);
}

void SmoothFloat::setRange(const float min, const float max) {
    minValue = std::min<float>(min, max);
    maxValue = std::max<float>(min, max);
}

SmoothFloat& SmoothFloat::operator = (const float v) {
    setValue(v);
    return *this;
}

float SmoothFloat::nextValue() {
    updateSmoothing();
    if (smoothing) {
        const float prevValue { currentValue };
        currentValue = targetValue * frac + currentValue * (1.0f - frac);

        if (fabsf(currentValue - prevValue) <= std::numeric_limits<float>::epsilon()) {
            // No advancement - jump to the target
            currentValue = targetValue;
        }
    }
    return currentValue;
}

void SmoothFloat::updateSmoothing() {
    smoothing = std::abs(currentValue - targetValue) > std::numeric_limits<float>::epsilon();
    if (!smoothing) {
        currentValue = targetValue;
    }
}

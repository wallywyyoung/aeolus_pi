// ----------------------------------------------------------------------------
//
//  Copyright (C) 2025 Wally Young <wallywyyoung@users.noreply.github.com>
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

#pragma once

#include <stdexcept>
#include <string>
#include <vector>

/**
 * Parameter with smoothed float value.
 */
class SmoothFloat
{
public:
    explicit SmoothFloat(float value = 0.0f, float min = 0.0f, float max = 1.0f, float smooth = 0.5f);

    void setName(const std::string& n) { paramName = n; }
    [[nodiscard]] const std::string& name() const noexcept { return paramName; }
    void setValue(float v, float s, bool force = false);
    void setValue(float v, bool force = false);
    void setSmoothing(float s) noexcept;
    void setRange(float min, float max);
    SmoothFloat& operator = (float v);

    [[nodiscard]] float value() const noexcept { return currentValue; }
    [[nodiscard]] float target() const noexcept { return targetValue; }
    [[nodiscard]] float min() const noexcept { return minValue; }
    [[nodiscard]] float max() const noexcept { return maxValue; }
    [[nodiscard]] bool isSmoothing() const noexcept { return smoothing || currentValue != targetValue; }
    [[nodiscard]] float nextValue();
    [[nodiscard]] float& targetRef() noexcept { return targetValue; }

private:
    void updateSmoothing();

    std::string paramName;   ///< Optional parameter name.

    float currentValue;
    float minValue;
    float maxValue;
    float targetValue;
    float frac;
    bool smoothing;
};

//----------------------------------------------------------

template <size_t NUMBER_PARAMETERS>
class SmoothFloatPool {
public:
    [[nodiscard]] static size_t size() { return NUMBER_PARAMETERS; }
    SmoothFloat& operator[] (int index) {
        if (index >= 0 && index < static_cast<int>(parameters.size())) return parameters.at(index);
        throw std::out_of_range("Index out of range");
    }
    const SmoothFloat& operator[] (int index) const {
        if (index >= 0 && index < static_cast<int>(parameters.size())) return parameters.at(index);
        throw std::out_of_range("Index out of range");
    }
    SmoothFloat& findByName(const std::string& n) {
        for (auto& parameter : parameters) {
            if (parameter.name() == n) {
                return parameter;
            }
        }
        throw std::out_of_range("Index out of range");
    }

private:
    std::array<SmoothFloat, NUMBER_PARAMETERS> parameters;
};


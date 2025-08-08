// ----------------------------------------------------------------------------
//
//  Copyright (C) 2025 Wally Young <wallywyyoung@users.noreply.github.com>
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
#include <bitset>
#include <memory>
#include <vector>

template<size_t numKeys>
struct Keyboard {
private:
    std::bitset<numKeys> keys;
    const int offset;
public:
    Keyboard() = delete;
    Keyboard(int offset) : offset(offset) { }

    bool press(int key) {
        auto index = key - offset;
        if (index < 0 || index >= numKeys) {
            return false;
        }
        return keys[index] = true;
    }

    bool release(int key) {
        auto index = key - offset;
        if (index < 0 || index >= numKeys) {
            return false;
        }
        return keys[index] = false;
    }

    bool isPressed(int key) {
        auto index = key - offset;
        if (index < 0 || index >= numKeys) { return false; }
        return keys[index];
    }
};
class KeyboardSystem {
    std::vector<std::unique_ptr<Keyboard>> keyboards;
};

struct Stop {
    bit enabled;
    vector<Key
};

#endif //KEYBOARDSYSTEM_H

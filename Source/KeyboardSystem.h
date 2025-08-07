//
// Created by Wally Young on 8/5/25.
//

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

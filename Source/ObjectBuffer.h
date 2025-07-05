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

#include <atomic>
#include <vector>
#include <new>

template <typename T>
class ObjectBuffer {
private:
    // Should be 64 on Raspberry Pi 4B.
    alignas(std::hardware_destructive_interference_size) std::atomic<size_t> _readIndex{0};
    alignas(std::hardware_destructive_interference_size) std::atomic<size_t> _writeIndex{0};
    alignas(std::hardware_destructive_interference_size) size_t _readIndexCached{0};
    alignas(std::hardware_destructive_interference_size) size_t _writeIndexCached{0};
    std::vector<T> buffer{};
public:
    explicit ObjectBuffer(size_t bufferSize = 1024) : buffer(1024, T()) { }
    ~ObjectBuffer() = default;
    ObjectBuffer(const ObjectBuffer&) = delete;
    ObjectBuffer& operator=(const ObjectBuffer&) = delete;

    bool push(T object) {
        auto const writeIndex = _writeIndex.load(std::memory_order_relaxed);
        auto nextWriteIndex = writeIndex + 1;
        if (nextWriteIndex == buffer.size()) {
            nextWriteIndex = 0;
        }
        if (nextWriteIndex == _readIndexCached) {
            _readIndexCached = _readIndex.load(std::memory_order_acquire);
            if (nextWriteIndex == _readIndexCached) {
                // Buffer is full.
                return false;
            }
        }
        buffer[writeIndex] = object;
        _writeIndex.store(nextWriteIndex, std::memory_order_release);
        return true;
    }

    bool push(std::vector<T> objects) {
        if (objects.size() > buffer.size()) {
            // Too many elements for buffer.
            return false;
        }
        auto const writeIndex = _writeIndex.load(std::memory_order_relaxed);
        auto nextWriteIndex = writeIndex + objects.size();
        auto wrapAround = false;

        if (nextWriteIndex > buffer.size()) {
            // Buffer wrap around.
            nextWriteIndex -= buffer.size();
            wrapAround = true;
        }

        if (nextWriteIndex == buffer.size()) {
            nextWriteIndex = 0;
        }

        if (nextWriteIndex == _readIndexCached || (wrapAround && nextWriteIndex > _readIndexCached)) {
            _readIndexCached = _readIndex.load(std::memory_order_acquire);
            if (nextWriteIndex == _readIndexCached || (wrapAround && nextWriteIndex > _readIndexCached)) {
                // Buffer is full.
                return false;
            }
        }

        if (nextWriteIndex > writeIndex) {
            buffer.insert(buffer.begin() + writeIndex, objects.begin(), objects.end());
        } else {
            size_t offset = objects.size() - nextWriteIndex - 1;
            buffer.insert(buffer.begin() + writeIndex, objects.begin(), objects.begin() + offset);
            ++offset;
            buffer.insert(buffer.begin(), objects.begin() + offset, objects.end());
        }
        _writeIndex.store(nextWriteIndex, std::memory_order_release);
        return true;
    }

    bool pop(T& object) {
        auto const readIndex = _readIndex.load(std::memory_order_relaxed);
        if (readIndex == _writeIndexCached) {
            _writeIndexCached = _writeIndex.load(std::memory_order_acquire);
            if (readIndex == _writeIndexCached) {
                // Buffer is empty.
                return false;
            }
        }
        object = buffer[readIndex];
        auto nextReadIndex = readIndex + 1;
        if (nextReadIndex == buffer.size()) {
            nextReadIndex = 0;
        }
        _readIndex.store(nextReadIndex, std::memory_order_release);
        return true;
    }

    bool pop(std::vector<T>& objects) {
        auto const readIndex = _readIndex.load(std::memory_order_relaxed);
        if (readIndex == _writeIndexCached) {
            _writeIndexCached = _writeIndex.load(std::memory_order_acquire);
            if (readIndex == _writeIndexCached) {
                // Buffer is empty.
                return false;
            }
        }
        if (readIndex < _writeIndexCached) {
            // Inline
            auto elements = readIndex - _writeIndexCached;
            objects.resize(elements);
            objects.insert(objects.begin(), buffer.begin() + readIndex, buffer.begin() + readIndex + elements - 1);
        } else {
            // Wrap Around
            auto offset = buffer.size() - readIndex;
            auto elements = offset + _writeIndexCached;
            objects.resize(elements);
            objects.insert(objects.begin(), buffer.begin() + readIndex, buffer.end());
            objects.insert(objects.begin() + offset, buffer.begin(), buffer.begin() + _writeIndexCached - 1);
        }
        // Buffer is cleared.
        const auto nextReadIndex = 0;
        _readIndex.store(nextReadIndex, std::memory_order_release);
        return true;
    }
};

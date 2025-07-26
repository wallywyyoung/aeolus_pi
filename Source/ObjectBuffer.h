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
#include <iostream>
#include <array>
#include <span>

template <typename T, size_t SIZE = 1024>
class ObjectBuffer {
    static constexpr size_t CACHE_LINE_SIZE = 64; // Should be 64 on Raspberry Pi 4B. Also consider std::hardware_destructive_interference_size
    alignas(CACHE_LINE_SIZE) std::atomic<size_t> _readIndex{0};
    alignas(CACHE_LINE_SIZE) std::atomic<size_t> _writeIndex{0};
    alignas(CACHE_LINE_SIZE) size_t _readIndexCached{0};
    alignas(CACHE_LINE_SIZE) size_t _writeIndexCached{0};
    std::array<T, SIZE> buffer{};
public:
    ObjectBuffer() = default;
    ~ObjectBuffer() = default;
    ObjectBuffer(const ObjectBuffer&) = delete;
    ObjectBuffer& operator=(const ObjectBuffer&) = delete;

    bool push(const T& object) noexcept {
        auto const writeIndex = _writeIndex.load(std::memory_order_relaxed);
        auto nextWriteIndex = (writeIndex + 1) % SIZE;
        if (nextWriteIndex == _readIndexCached) {
            _readIndexCached = _readIndex.load(std::memory_order_acquire);
            if (nextWriteIndex == _readIndexCached) {
                // Buffer is full.
                std::cerr << "ObjectBuffer::push - Fail logging event, buffer full." << std::endl;
                return false;
            }
        }
        buffer[writeIndex] = object;
        _writeIndex.store(nextWriteIndex, std::memory_order_release);
        std::cout << "ObjectBuffer::push - Successfully logging event." << std::endl;
        return true;
    }

    bool push(std::span<const T> objects) noexcept {
        if (objects.size() > buffer.size()) {
            // Too many elements for buffer.
            return false;
        }
        auto const writeIndex = _writeIndex.load(std::memory_order_relaxed);
        auto nextWriteIndex = writeIndex + objects.size();
        bool const wrapAround = nextWriteIndex > SIZE;

        if (wrapAround) {
            nextWriteIndex %= SIZE;
        }

        if (nextWriteIndex == _readIndexCached || (wrapAround && nextWriteIndex > _readIndexCached)) {
            _readIndexCached = _readIndex.load(std::memory_order_acquire);
            if (nextWriteIndex == _readIndexCached || (wrapAround && nextWriteIndex > _readIndexCached)) {
                // Buffer is full.
                return false;
            }
        }

        if (nextWriteIndex > writeIndex) {
            std::copy(objects.begin(), objects.end(), buffer.begin() + writeIndex);
        } else {
            size_t offset = objects.size() - nextWriteIndex;
            std::copy(objects.begin(), objects.begin() + offset, buffer.begin() + writeIndex);
            std::copy(objects.begin() + offset, objects.end(), buffer.begin());
        }
        _writeIndex.store(nextWriteIndex, std::memory_order_release);
        return true;
    }

    bool pop(T& object) noexcept {
        auto const readIndex = _readIndex.load(std::memory_order_relaxed);
        if (readIndex == _writeIndexCached) {
            _writeIndexCached = _writeIndex.load(std::memory_order_acquire);
            if (readIndex == _writeIndexCached) {
                // Buffer is empty.
                return false;
            }
        }
        object = buffer[readIndex];
        auto nextReadIndex = (readIndex + 1) % SIZE;
        _readIndex.store(nextReadIndex, std::memory_order_release);
        return true;
    }

    bool pop(std::span<const T>& objects) noexcept {
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
            auto elements =  _writeIndexCached - readIndex;
            objects.resize(elements);
            objects.insert(objects.begin(), buffer.begin() + readIndex, buffer.begin() + readIndex + elements);
        } else {
            // Wrap Around
            auto offset = buffer.size() - readIndex;
            auto elements = offset + _writeIndexCached;
            objects.resize(elements);
            objects.insert(objects.begin(), buffer.begin() + readIndex, buffer.end());
            objects.insert(objects.begin() + offset, buffer.begin(), buffer.begin() + _writeIndexCached);
        }
        // Buffer is cleared.
        const auto nextReadIndex = 0;
        _readIndex.store(nextReadIndex, std::memory_order_release);
        std::cout << "ObjectBuffer::pop - Successfully popped events." << std::endl;
        return true;
    }
};

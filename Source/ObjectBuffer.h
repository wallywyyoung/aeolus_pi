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

#include "MemoryGlobal.h"

#include <atomic>
#include <iostream>
#include <array>
#include <vector>

// SPSC Lock-Free Ringbuffer
template <typename T, size_t SIZE = 1024>
class ObjectBuffer {
    // TODO: Perf test and consider all vars in a single cacheline.
    struct alignas(CACHE_LINE_SIZE) ConsumerFields {
        std::atomic<size_t> readIndex{0};
        size_t writeIndexCached{0};
    };

    struct alignas(CACHE_LINE_SIZE) ProducerFields {
        std::atomic<size_t> writeIndex{0};
        size_t readIndexCached{0};
    };

    ProducerFields producerFields{};
    ConsumerFields consumerFields{};
    std::array<T, SIZE> buffer{};
public:
    ObjectBuffer() = default;
    ~ObjectBuffer() = default;
    ObjectBuffer(const ObjectBuffer&) = delete;
    ObjectBuffer& operator=(const ObjectBuffer&) = delete;

    bool push(const T& object) noexcept {
        auto const writeIndex = producerFields.writeIndex.load(std::memory_order_relaxed);
        auto nextWriteIndex = (writeIndex + 1) % SIZE;
        if (nextWriteIndex == producerFields.readIndexCached) {
            producerFields.readIndexCached = consumerFields.readIndex.load(std::memory_order_acquire);
            if (nextWriteIndex == producerFields.readIndexCached) {
                // Buffer is full.
                std::cerr << "ObjectBuffer::push - Fail logging event, buffer full." << std::endl;
                return false;
            }
        }
        buffer[writeIndex] = object;
        producerFields.writeIndex.store(nextWriteIndex, std::memory_order_release);
        return true;
    }

    bool push(std::vector<T> objects) noexcept {
        if (objects.size() > buffer.size()) {
            // Too many elements for buffer.
            return false;
        }
        auto const writeIndex = producerFields.writeIndex.load(std::memory_order_relaxed);
        auto nextWriteIndex = writeIndex + objects.size();
        bool const wrapAround = nextWriteIndex > SIZE;

        if (wrapAround) {
            nextWriteIndex %= SIZE;
        }

        if (nextWriteIndex == producerFields.readIndexCached || (wrapAround && nextWriteIndex > producerFields.readIndexCached)) {
            producerFields.readIndexCached = consumerFields.readIndex.load(std::memory_order_acquire);
            if (nextWriteIndex == producerFields.readIndexCached || (wrapAround && nextWriteIndex > producerFields.readIndexCached)) {
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
        producerFields.writeIndex.store(nextWriteIndex, std::memory_order_release);
        return true;
    }

    bool pop(T& object) noexcept {
        auto const readIndex = consumerFields.readIndex.load(std::memory_order_relaxed);
        if (readIndex == consumerFields.writeIndexCached) {
            consumerFields.writeIndexCached = producerFields.writeIndex.load(std::memory_order_acquire);
            if (readIndex == consumerFields.writeIndexCached) {
                // Buffer is empty.
                return false;
            }
        }
        object = buffer[readIndex];
        auto nextReadIndex = (readIndex + 1) % SIZE;
        consumerFields.readIndex.store(nextReadIndex, std::memory_order_release);
        return true;
    }

    bool pop(std::vector<T>& objects) noexcept {
        auto const readIndex = consumerFields.readIndex.load(std::memory_order_relaxed);
        if (readIndex == consumerFields.writeIndexCached) {
            consumerFields.writeIndexCached = producerFields.writeIndex.load(std::memory_order_acquire);
            if (readIndex == consumerFields.writeIndexCached) {
                // Buffer is empty.
                return false;
            }
        }
        objects.clear();
        if (readIndex < consumerFields.writeIndexCached) {
            // Inline
            auto elements =  consumerFields.writeIndexCached - readIndex;
            objects.reserve(elements);
            for (size_t i = readIndex; i < consumerFields.writeIndexCached; ++i) {
                objects.push_back(buffer[i]);
            }
        } else {
            // Wrap Around
            auto offset = buffer.size() - readIndex;
            auto elements = offset + consumerFields.writeIndexCached;
            objects.reserve(elements);
            for (size_t i = readIndex; i < SIZE; ++i) {
                objects.push_back(buffer[i]);
            }
            for (size_t i = 0; i < consumerFields.writeIndexCached; ++i) {
                objects.push_back(buffer[i]);
            }
        }
        // Buffer is cleared.
        consumerFields.readIndex.store(consumerFields.writeIndexCached, std::memory_order_release);
        return true;
    }
};

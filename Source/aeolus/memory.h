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

#pragma once

#include <memory>


/**
 * Allocate memory with predefined alignment.
 *
 * Ported from https://embeddedartistry.com/blog/2017/2/20/implementing-aligned-malloc
 */
template <size_t Align>
struct AlignedMemory
{
    typedef uint16_t offset_t;
    constexpr static size_t PTR_OFFSET_SIZE = sizeof(offset_t);
    constexpr static size_t alignment = Align;

    /// Allocate aligned memory block.
    static void* alloc(const size_t size)
    {
        void* ptr = nullptr;

        if (alignment && size) {
            const uint32_t hdr_size = PTR_OFFSET_SIZE + (alignment - 1);

            if (void* p = ::malloc (size + hdr_size)) {
                ptr = reinterpret_cast<void *>(alignUp((reinterpret_cast<uintptr_t>(p) + PTR_OFFSET_SIZE)));

                //Calculate the offset and store it behind our aligned pointer
                *(static_cast<offset_t *>(ptr) - 1) = static_cast<offset_t>(reinterpret_cast<uintptr_t>(ptr) - reinterpret_cast<uintptr_t>(p));
            }
        }

        return ptr;
    }

    /// Release aligned memory block allocated by \ref alloc()
    static void free(void* ptr)
    {
        if (ptr) {
            const offset_t offset = *(static_cast<offset_t *>(ptr) - 1);

            // Once we have the offset, we can get our original pointer and call free
            auto p = static_cast<void *>(static_cast<uint8_t *>(ptr) - offset);
            ::free (p);
        }
    }

private:

    inline static size_t alignUp(const size_t num)
    {
        return (num + (alignment - 1)) & ~(alignment - 1);
    }

    AlignedMemory()
    {
        static_assert ((alignment & (alignment - 1)) == 0);
    }
};



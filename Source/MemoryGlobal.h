//
// Created by Wally Young on 7/25/25.
//

#pragma once
#include <cstddef>

// Should be 64 on Raspberry Pi 4B. Also consider std::hardware_destructive_interference_size
static constexpr std::size_t CACHE_LINE_SIZE = 64;
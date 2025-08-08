##  Copyright (C) 2025 Wally Young <wallywyyoung@users.noreply.github.com>
##  Copyright (C) 2021 Arthur Benilov <arthur.benilov@gmail.com>
##
##  This program is free software; you can redistribute it and/or modify
##  it under the terms of the GNU General Public License as published by
##  the Free Software Foundation; either version 3 of the License, or
##  (at your option) any later version.
##
##  This program is distributed in the hope that it will be useful,
##  but WITHOUT ANY WARRANTY; without even the implied warranty of
##  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
##  GNU General Public License for more details.
##
##  You should have received a copy of the GNU General Public License
##  along with this program.  If not, see <http://www.gnu.org/licenses/>.

cmake_minimum_required(VERSION 3.25)
project(aeolus_pi)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=armv8-a -mcpu=cortex-a72")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -Ofast")

include_directories(Source)

add_executable(aeolus_pi Source/main.cpp
        Source/MemoryUtilities.cpp
        Source/MemoryUtilities.h
        Source/MidiData.cpp
        Source/MidiData.h
        Source/ObjectBuffer.h
        Source/StaticAudioBuffer.h
        Source/AlsaInterface.cpp
        Source/AlsaInterface.h
        Source/AudioBuffer.cpp
        Source/AudioBuffer.h
        Source/IOManager.cpp
        Source/IOManager.h
        Source/aeolus/AudioParameter.cpp
        Source/aeolus/AudioParameter.h
        Source/aeolus/Division.cpp
        Source/aeolus/Division.h
        Source/aeolus/DivisionFactory.cpp
        Source/aeolus/DivisionFactory.h
        Source/aeolus/EngineGlobal.cpp
        Source/aeolus/globals.cpp
        Source/aeolus/HN_func.cpp
        Source/aeolus/HN_func.h
        Source/aeolus/list.h
        Source/aeolus/memory.h
        Source/aeolus/Model.cpp
        Source/aeolus/Model.h
        Source/aeolus/N_func.cpp
        Source/aeolus/N_func.h
        Source/aeolus/Organ.cpp
        Source/aeolus/Pipewave.cpp
        Source/aeolus/Pipewave.h
        Source/aeolus/Rankwave.cpp
        Source/aeolus/Scale.cpp
        Source/aeolus/Scale.h
        Source/aeolus/Semaphore.cpp
        Source/aeolus/Semaphore.h
        Source/aeolus/Sequencer.cpp
        Source/aeolus/SIMD.cpp
        Source/aeolus/SIMD.h
        Source/aeolus/Stop.cpp
        Source/aeolus/Stop.h
        Source/aeolus/Voice.cpp
        Source/aeolus/VoicePool.cpp
        Source/aeolus/Worker.cpp
        Source/aeolus/Worker.h
        Source/aeolus/utilities/Range.cpp
        Source/aeolus/dsp/adsrenv.cpp
        Source/aeolus/dsp/chiff.cpp
        Source/aeolus/dsp/convolve.h
        Source/aeolus/dsp/convolver.cpp
        Source/aeolus/dsp/delay.cpp
        Source/aeolus/dsp/fft.cpp
        Source/aeolus/dsp/fft.h
        Source/aeolus/dsp/filter.cpp
        Source/aeolus/dsp/interpolator.cpp
        Source/aeolus/dsp/interpolator.h
        Source/aeolus/dsp/spatial.cpp)

find_package(nlohmann_json CONFIG REQUIRED)
find_package(AudioFile CONFIG REQUIRED)
find_package(thread-pool CONFIG REQUIRED)
find_package(ALSA REQUIRED)
target_link_libraries(aeolus_pi PRIVATE nlohmann_json::nlohmann_json AudioFile::AudioFile dp::thread-pool ALSA::ALSA)

file(COPY Resources DESTINATION ${CMAKE_BINARY_DIR})

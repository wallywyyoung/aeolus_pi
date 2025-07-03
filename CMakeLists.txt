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

# TODO: Put this where it belongs... toolchain file?
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=armv8-a -mcpu=cortex-a72")

option(WITH_MULTIBUS_OUTPUT "Enable multibus output" OFF)

include_directories(Source)

add_executable(aeolus_pi
        Source/aeolus/dsp/convolve.h)

target_sources(aeolus_pi PRIVATE
        Source/aeolus/utilities/Range.cpp
        Source/aeolus/utilities/Range.h
        Source/aeolus/dsp/adsrenv.cpp
        Source/aeolus/dsp/adsrenv.h
        Source/aeolus/dsp/chiff.cpp
        Source/aeolus/dsp/chiff.h
        Source/aeolus/dsp/convolve.h
        Source/aeolus/dsp/convolver.cpp
        Source/aeolus/dsp/convolver.h
        Source/aeolus/dsp/delay.cpp
        Source/aeolus/dsp/delay.h
        Source/aeolus/dsp/fft.cpp
        Source/aeolus/dsp/fft.h
        Source/aeolus/dsp/filter.cpp
        Source/aeolus/dsp/filter.h
        Source/aeolus/dsp/interpolator.cpp
        Source/aeolus/dsp/interpolator.h
        Source/aeolus/dsp/spatial.cpp
        Source/aeolus/dsp/spatial.h
        Source/aeolus/Addsynth.cpp
        Source/aeolus/Addsynth.h
        Source/aeolus/audioparam.cpp
        Source/aeolus/audioparam.h
        Source/aeolus/Division.cpp
        Source/aeolus/Division.h
        Source/aeolus/DivisionFactory.cpp
        Source/aeolus/DivisionFactory.h
        Source/aeolus/engine.cpp
        Source/aeolus/engine.h
        Source/aeolus/globals.cpp
        Source/aeolus/globals.h
        Source/aeolus/list.h
        Source/aeolus/memory.h
        Source/aeolus/MidiManager.h
        Source/aeolus/MidiMessage.h
        Source/aeolus/Model.h
        Source/aeolus/Model.cpp
        Source/aeolus/rankwave.cpp
        Source/aeolus/rankwave.h
        Source/aeolus/ringbuffer.h
        Source/aeolus/scale.cpp
        Source/aeolus/scale.h
        Source/aeolus/sema.cpp
        Source/aeolus/sema.h
        Source/aeolus/sequencer.cpp
        Source/aeolus/sequencer.h
        Source/aeolus/simd.cpp
        Source/aeolus/simd.h
        Source/aeolus/stop.cpp
        Source/aeolus/stop.h
        Source/aeolus/voice.cpp
        Source/aeolus/voice.h
        Source/aeolus/worker.cpp
        Source/aeolus/worker.h
        Source/aeolus/HN_func.cpp
        Source/aeolus/HN_func.h
        Source/aeolus/N_func.cpp
        Source/aeolus/N_func.h
        Source/aeolus/Pipewave.cpp
        Source/aeolus/Pipewave.h
        Source/aeolus/EngineGlobal.cpp
        Source/aeolus/EngineGlobal.h
        Source/mts/libMTSClient.cpp
        Source/mts/libMTSClient.h
        Source/AudioBuffer.cpp
        Source/AudioBuffer.h
        Source/AlsaInterface.cpp
        Source/AlsaInterface.h
        Source/IOManager.cpp
        Source/IOManager.h
        Source/main.cpp)

find_package(nlohmann_json CONFIG REQUIRED)
find_package(AudioFile CONFIG REQUIRED)
find_package(thread-pool CONFIG REQUIRED)
find_package(ALSA REQUIRED)
target_link_libraries(aeolus_pi PRIVATE nlohmann_json::nlohmann_json AudioFile::AudioFile dp::thread-pool ALSA::ALSA)

file(COPY Resources DESTINATION ${CMAKE_BINARY_DIR})

if(WITH_MULTIBUS_OUTPUT)
    target_compile_definitions(aeolus_pi PUBLIC AEOLUS_MULTIBUS_OUTPUT=1)
else()
    target_compile_definitions(aeolus_pi PUBLIC AEOLUS_MULTIBUS_OUTPUT=0)
endif()

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

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mcpu=cortex-a72 -march=armv8-a+simd")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -ffast-math -DARM_MATH_LOOPUNROLL")

include_directories(Source)

add_executable(aeolus_pi
        Source/EngineGlobal.cpp
        Source/EngineGlobal.h
        Source/main.cpp
        Source/MemoryConstants.h
        Source/MidiData.cpp
        Source/MidiData.h
        Source/ObjectBuffer.h
        Source/StaticAudioBuffer.h
        Source/AudioBuffer.cpp
        Source/AudioBuffer.h
        Source/IOManager.cpp
        Source/IOManager.h
        Source/aeolus/AddSynth.h
        Source/aeolus/SmoothFloat.cpp
        Source/aeolus/SmoothFloat.h
        Source/aeolus/Division.cpp
        Source/aeolus/Division.h
        Source/aeolus/DivisionFactory.cpp
        Source/aeolus/DivisionFactory.h
        Source/aeolus/HN_func.cpp
        Source/aeolus/HN_func.h
        Source/aeolus/memory.h
        Source/aeolus/Model.cpp
        Source/aeolus/Model.h
        Source/aeolus/N_func.cpp
        Source/aeolus/N_func.h
        Source/aeolus/Organ.cpp
        Source/aeolus/StaticPipe.cpp
        Source/aeolus/StaticPipe.h
        Source/aeolus/RankWave.cpp
        Source/aeolus/RankWave.h
        Source/aeolus/Scale.cpp
        Source/aeolus/Scale.h
        Source/aeolus/Semaphore.cpp
        Source/aeolus/Semaphore.h
        Source/aeolus/Sequencer.cpp
        Source/aeolus/SIMD.cpp
        Source/aeolus/SIMD.h
        Source/aeolus/Stop.h
        Source/aeolus/Voice.cpp
        Source/aeolus/Worker.cpp
        Source/aeolus/Worker.h
        Source/aeolus/utilities/Range.cpp
        Source/aeolus/dsp/Envelope.cpp
        Source/aeolus/dsp/Envelope.h
        Source/aeolus/dsp/Chiff.cpp
        Source/aeolus/dsp/Chiff.h
        Source/aeolus/dsp/SpatialSource.cpp
        Source/aeolus/StopFactory.cpp
        Source/aeolus/utilities/SimdUtilities.cpp
        Source/aeolus/utilities/SimdUtilities.h
        Source/aeolus/dsp/DelayLineStatic.h
        Source/AlsaMidiInterface.h
        Source/AlsaMidiInterface.cpp
        Source/RtAudioInterface.h
        Source/RtAudioInterface.cpp
        Source/RestManager.h
        Source/RestManager.cpp
        Source/aeolus/PipeState.h
        Source/aeolus/WavetableMemoryManager.h
        Source/aeolus/WavetableMemoryManager.cpp
        Source/aeolus/PipeState.cpp
        Source/LowPassFilter.h
        Source/LowPassFilter.cpp
        Source/aeolus/dsp/StereoFIRConvolver.h
        Source/aeolus/dsp/StereoPartitionedConvolver.h
        Source/aeolus/dsp/Convolver.h
        Source/aeolus/dsp/StereoFIRConvolver.cpp
        Source/aeolus/dsp/StereoPartitionedConvolver.cpp
)

find_package(nlohmann_json CONFIG REQUIRED)
find_package(AudioFile CONFIG REQUIRED)
find_package(RtAudio CONFIG REQUIRED)
set(CMSIS_DSP_DIR ${CMAKE_SOURCE_DIR}/External/CMSIS-DSP)
include_directories(${CMSIS_DSP_DIR}/include)

if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    target_compile_definitions(aeolus_pi PRIVATE MACOS)
    target_link_libraries(aeolus_pi PRIVATE ${CMSIS_DSP_DIR}/lib/macOS/libCMSISDSP.a)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_sources(aeolus_pi PRIVATE
            Source/AlsaAudioInterface.cpp
            Source/AlsaAudioInterface.h)
    find_package(ALSA REQUIRED)
    target_link_libraries(aeolus_pi PRIVATE ALSA::ALSA ${CMSIS_DSP_DIR}/lib/Linux/libCMSISDSP.a)
    target_compile_definitions(aeolus_pi PRIVATE LINUX)
endif()

target_link_libraries(aeolus_pi PRIVATE nlohmann_json::nlohmann_json AudioFile::AudioFile RtAudio::rtaudio)
target_compile_definitions(aeolus_pi PRIVATE $<$<CONFIG:Debug>:DEBUG>)

file(COPY Resources DESTINATION ${CMAKE_BINARY_DIR})

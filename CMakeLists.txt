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

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

option(WITH_MULTIBUS_OUTPUT "Enable multibus output" OFF)

include_directories(Source)
include_directories(Source/aeolus)
include_directories(Source/aeolus/dsp)
include_directories(Source/mts)
include_directories(Source/ui)

find_package(JUCE CONFIG REQUIRED)
juce_add_console_app(aeolus_pi
        VERSION "0.2.1"
#        PLUGIN_MANUFACTURER_CODE "Arbe"
#        PLUGIN_CODE "Arbe"
#        PLUGIN_DESCRIPTION "Aeolus"
#        PRODUCT_NAME "Aeolus"
#        COMPANY_NAME "Arthur Benilov"
#        BUNDLE_ID "com.ArthurBenilov.Aeolus"
#        IS_SYNTH TRUE
#        IS_MIDI_EFFECT FALSE
#        NEEDS_MIDI_INPUT TRUE
#        FORMATS standalone
#        LV2URI "https://github.com/Archie3d/aeolus_plugin"
#        ICON_BIG "${CMAKE_CURRENT_SOURCE_DIR}/Resources/icons/icon256.png"
#        ICON_SMALL "${CMAKE_CURRENT_SOURCE_DIR}/Resources/icons/icon64.png"
)

#Required for JuceHeader.h
juce_generate_juce_header(aeolus_pi)

#Required for BinaryData
file(GLOB_RECURSE resources "${CMAKE_CURRENT_SOURCE_DIR}/Resources/*.*")
juce_add_binary_data(aeolus_pi_resources SOURCES ${resources})

target_link_libraries(aeolus_pi
        PUBLIC
        aeolus_pi_resources
        PRIVATE
        juce::juce_core
        juce::juce_data_structures
        juce::juce_audio_basics
        juce::juce_audio_utils
        juce::juce_audio_plugin_client
        juce::juce_gui_basics
        juce::juce_gui_extra
        juce::juce_recommended_config_flags)

target_sources(aeolus_pi PRIVATE
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
        Source/aeolus/addsynth.cpp
        Source/aeolus/addsynth.h
        Source/aeolus/audioparam.cpp
        Source/aeolus/audioparam.h
        Source/aeolus/division.cpp
        Source/aeolus/division.h
        Source/aeolus/engine.cpp
        Source/aeolus/engine.h
        Source/aeolus/globals.cpp
        Source/aeolus/globals.h
        Source/aeolus/levelmeter.cpp
        Source/aeolus/levelmeter.h
        Source/aeolus/list.h
        Source/aeolus/memory.h
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
        Source/Parameters.cpp
        Source/Parameters.h
        Source/PluginProcessor.cpp
        Source/PluginProcessor.h
        Source/mts/libMTSClient.cpp
        Source/mts/libMTSClient.h
        Source/main.cpp)

if(WITH_MULTIBUS_OUTPUT)
    target_compile_definitions(aeolus_pi PUBLIC AEOLUS_MULTIBUS_OUTPUT=1 JUCE_USE_CUSTOM_PLUGIN_STANDALONE_APP=1)
else()
    target_compile_definitions(aeolus_pi PUBLIC AEOLUS_MULTIBUS_OUTPUT=0 JUCE_USE_CUSTOM_PLUGIN_STANDALONE_APP=1)
endif()

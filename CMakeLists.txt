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
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR aarch64)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

option(WITH_MULTIBUS_OUTPUT "Enable multibus output" OFF)

find_package(JUCE CONFIG REQUIRED)
target_link_libraries(main PRIVATE
        juce::juce_core
        juce::juce_data_structures
        juce::juce_audio_basics
        juce::juce_audio_utils
        juce::juce_audio_plugin_client
        juce::juce_gui_basics
        juce::juce_gui_extra
        juce::juce_recommended_config_flags)

include_directories(Source)
include_directories(Source/aeolus)
include_directories(Source/aeolus/dsp)
include_directories(Source/mts)
include_directories(Source/ui)

add_executable(aeolus_pi
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
        Source/mts/libMTSClient.cpp
        Source/mts/libMTSClient.h
        Source/ui/CustomLookAndFeel.cpp
        Source/ui/CustomLookAndFeel.h
        Source/ui/CustomMidiKeyboard.cpp
        Source/ui/CustomMidiKeyboard.h
        Source/ui/DivisionControlPanel.cpp
        Source/ui/DivisionControlPanel.h
        Source/ui/DivisionView.cpp
        Source/ui/DivisionView.h
        Source/ui/GlobalTuningComponent.cpp
        Source/ui/GlobalTuningComponent.h
        Source/ui/LevelIndicator.cpp
        Source/ui/LevelIndicator.h
        Source/ui/MidiChannelsComponent.cpp
        Source/ui/MidiChannelsComponent.h
        Source/ui/MidiChannelsSelectComponent.cpp
        Source/ui/MidiChannelsSelectComponent.h
        Source/ui/OverlayComponent.cpp
        Source/ui/OverlayComponent.h
        Source/ui/ParameterSlider.cpp
        Source/ui/ParameterSlider.h
        Source/ui/SequencerView.cpp
        Source/ui/SequencerView.h
        Source/ui/SettingsComponent.cpp
        Source/ui/SettingsComponent.h
        Source/ui/StopButton.cpp
        Source/ui/StopButton.h
        Source/Parameters.cpp
        Source/Parameters.h
        Source/PluginEditor.cpp
        Source/PluginEditor.h
        Source/PluginProcessor.cpp
        Source/PluginProcessor.h)

file(GLOB_RECURSE res "${CMAKE_CURRENT_SOURCE_DIR}/Resources/*.*")

if(WITH_MULTIBUS_OUTPUT)
    target_compile_definitions(aeolus_pi PUBLIC AEOLUS_MULTIBUS_OUTPUT=1)
else()
    target_compile_definitions(aeolus_pi PUBLIC AEOLUS_MULTIBUS_OUTPUT=0)
endif()

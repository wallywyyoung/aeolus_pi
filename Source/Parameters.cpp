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

#include "AeolusAudioProcessor.h"

#include "aeolus/division.h"

#include "Parameters.h"

Parameters::Parameters(AeolusAudioProcessor& proc) : processor(proc) {
    processor.addParameter(reverbWet = new AudioParameterFloat(ParameterID{"reverb_wet", 1}, "Reverb", 0.0f, 1.0f, 0.25f));
    processor.addParameter(volume = new AudioParameterFloat(ParameterID{"volume", 1}, "Volume", 0.0f, 1.0f, 0.5f));

    auto& engine = proc.getEngine();

    for (int i = 0; i < engine.getDivisionCount(); ++i) {
        auto* division = engine.getDivisionByIndex(i);

        auto param = std::make_unique<AudioParameterFloat>(ParameterID{String("gain_") + String(i), 1}, division->getName() + " gain", 0.0f, 1.0f, 0.5f);
        auto* ptr = param.get();
        processor.addParameter(param.release());
        divisionsGain.push_back(ptr);
        division->setParamGain(ptr);
    }
}

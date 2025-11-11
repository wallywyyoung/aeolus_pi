// ----------------------------------------------------------------------------
//
//  Copyright (C) 2025 Wally Young <wallywyyoung@users.noreply.github.com>
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

#include "EngineGlobal.h"
#include <thread>
#include "IOManager.h"
#include "aeolus/Organ.h"
#include "aeolus/RankWave.h"
#include "aeolus/utilities/SimdUtilities.h"

EngineGlobal::EngineGlobal() {
    irs = IOManager::loadIRs();
    for (auto i = 0; i <  model.getStopsCount(); ++i) {
        // TODO: Fix this mapping in JSON.
        _rankwavesByName.emplace(model[i].getFileName(), std::make_shared<RankWave>(model[i], *scale, tuningFrequency));
    }
    generateWavetables();
    organ = std::make_shared<Organ>([this](const std::string &name) { return getStopByName(name); });
    organInterface = static_cast<OrganInterface *>(organ.get());
    convolver.setIR(irs.irs[0]);
}

void EngineGlobal::process(float (&out)[PROCESS_SAMPLES_SIZE]) {
    // Midi / Configuration Block
    ProcessMidiBuffer();
    // Organ Block
    const bool wasAudioGenerated = organ->process(out);
    // Reverb Block
    convolver.process<PROCESS_FRAMES_SIZE>(out, wasAudioGenerated);
}

void EngineGlobal::generateWavetables() const {
    auto threads = std::vector<std::thread>();
    for (const auto &val: _rankwavesByName | std::views::values) {
        auto rwp = val.get();
        threads.emplace_back([rwp] {
            SimdUtilities::enableFlushToZero();
            rwp->generateWavetables();
            SimdUtilities::disableFlushToZero();
        });
    }
    for (auto &t : threads) {
        t.join();
    }
}

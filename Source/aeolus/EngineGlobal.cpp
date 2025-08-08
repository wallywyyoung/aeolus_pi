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

#include "aeolus/EngineGlobal.h"
#include "IOManager.h"
#include "aeolus/Organ.h"

#include <thread_pool/thread_pool.h>

EngineGlobal::EngineGlobal() : _scale(std::make_shared<Scale>(Scale(Scale::EqualTemp))), _tuningFrequency(TUNING_FREQUENCY_DEFAULT) { }

void EngineGlobal::init() {
    irs = IOManager::loadIRs();
    loadRankwaves();
    updateStops();
    engine = new Organ();
    engine->prepareToPlay();
    engine->setGlobalAllStopsOn();
    engine->setVolume(0.005f, true);
}

std::vector<std::string> EngineGlobal::getAllStopNames() const
{
    auto names = std::vector<std::string>();

    for (const auto &key: _rankwavesByName | std::views::keys) {
        names.push_back(key);
    }

    return names;
}

void EngineGlobal::updateStops() const {
    dp::thread_pool pool(_rankwavesByName.size());
    for (const auto &val: _rankwavesByName | std::views::values) {
        auto rwp = val.get();
        pool.enqueue_detach([rwp] {
            MemoryUtilities::enableFlushToZero();
            rwp->prepareToPlay();
            MemoryUtilities::disableFlushToZero();
        });
    }
    pool.wait_for_tasks();
}

void EngineGlobal::rebuildRankwaves() {
    // Prepare all the rankwaves to be retuned
    for (const auto &val: _rankwavesByName | std::views::values) {
        val->retunePipes(*_scale, _tuningFrequency);
    }

    // @note We don't kill active voices - they will be using pipes from a parallel set.
    //       However, switching tuning very fast (while keeping the voice sustained)
    //       may result in voice to be killed.
    updateStops();
}

void EngineGlobal::pushMidi(const MidiData &midi) {
    engine->push(midi);
}

void EngineGlobal::pushMidi(const std::vector<MidiData> &midi) {
    engine->push(midi);
}

void EngineGlobal::loadRankwaves() {
    for (int i = 0; i <  model.getStopsCount(); ++i) {
        // TODO: Fix this mapping in JSON.
        _rankwavesByName.emplace(model[i].getFileName(), std::make_unique<Rankwave>(model[i], *_scale, _tuningFrequency));
    }
}

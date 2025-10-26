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
#include "IOManager.h"
#include "aeolus/Organ.h"
#include "aeolus/utilities/SimdUtilities.h"
#include <thread>

EngineGlobal::EngineGlobal() {
    irs = IOManager::loadIRs();
    for (auto i = 0; i <  model.getStopsCount(); ++i) {
        // TODO: Fix this mapping in JSON.
        _rankwavesByName.emplace(model[i].getFileName(), std::make_shared<RankWave>(model[i], *scale, tuningFrequency));
    }
    generateWavetables();
    organ = new Organ([this](const std::string &name) { return getStopByName(name); });
    organInterface = static_cast<OrganInterface *>(organ);
    convolver.setIR(irs.irs[0]);
    std::cout << "Setting debug stop/note on" << std::endl;
    organ->setDivisionStopOn(1,0);
    // organ->setDivisionStopOn(1,1);
    // organ->setDivisionStopOn(1,2);
    // organ->setDivisionStopOn(1,3);
    // organ->setDivisionStopOn(1,4);
    // organ->setDivisionStopOn(1,5);
    // organ->setDivisionStopOn(1,6);
    // organ->setDivisionStopOn(1,7);
    // organ->setDivisionStopOn(1,8);
    // organ->setDivisionStopOn(1,9);
    // organ->setDivisionStopOn(1,10); // BROKEN
    // organ->setDivisionStopOn(1,11); // BROKEN
    // organ->setDivisionStopOn(1,12); // BROKEN
    // organ->setDivisionStopOn(1,13); // BROKEN
    // organ->setDivisionNoteOn(0,50);
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

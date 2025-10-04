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

#pragma once

#include "ObjectBuffer.h"
#include "MidiData.h"

class MidiManager : public ObjectBuffer<MidiData> {
public:
    class OrganInterface {
    public:
        virtual ~OrganInterface() = default;

        // Notes
        virtual void setDivisionNoteOn(const int& division, const int& note) = 0;
        virtual void setDivisionNoteOff(const int& division, const int& note) = 0;
        virtual void setDivisionAllNotesOff(const int& division) = 0;
        virtual void setGlobalAllNotesOff() = 0;
        // Swell
        virtual void handleDivisionSwell(const int& division, const float& value) = 0;
        // Stops
        virtual void setDivisionStopOn(const int& division, const int& stop) = 0;
        virtual void setDivisionStopOff(const int& division, const int& stop) = 0;
        virtual void setDivisionStopToggle(const int& division, const int& stop) = 0;
        virtual void setDivisionAllStopsOff(const int& division) = 0;
        virtual void setDivisionAllStopsOn(const int& division) = 0;
        virtual void setGlobalAllStopsOff() = 0;
        virtual void setGlobalAllStopsOn() = 0;
        // Couplers
        virtual void setDivisionCouplerOn(const int& division, const int& coupler) = 0;
        virtual void setDivisionCouplerOff(const int& division, const int& coupler) = 0;
        // Tremulant
        virtual void setDivisionTremulantOn(const int& division) = 0;
        virtual void setDivisionTremulantOff(const int& division) = 0;
        // Pistons
        virtual void setDivisionPiston(const int& division, const int& piston) = 0;
        virtual void recallDivisionPiston(const int& division, const int& piston) = 0;
        virtual void setGlobalPiston(const int& piston) = 0;
        virtual void recallGlobalPiston(const int& piston) = 0;
    };

    MidiManager() : ObjectBuffer() { }

    void ProcessMidiBuffer() {
        std::vector<MidiData> midiBuffer{};
        this->pop(midiBuffer);
        for (const MidiData& event : midiBuffer) {
            ProcessMidiEvent(event);
        }
    }

protected:
    OrganInterface* organInterface = nullptr;

private:
    void handlePC(const MidiData& event) {
        enum PistonControl {
            RecallDivisionPiston,
            SetDivisionPiston,
            RecallGlobalPiston,
            SetGlobalPiston,
            StopOff,
            StopOn,
            StopToggle,
            AllDivisionStopsOff,
            AllDivisionStopsOn,
            AllGlobalStopsOff,
            AllGlobalStopsOn,
            DivisionCouplerOn,
            DivisionCouplerOff,
            DivisionTremulantOn,
            DivisionTremulantOff,
        };
        switch (static_cast<PistonControl>(event.param)) {
            case RecallDivisionPiston:
                organInterface->recallDivisionPiston(event.channel, event.value);
                break;
            case SetDivisionPiston:
                organInterface->setDivisionPiston(event.channel, event.value);
                break;
            case RecallGlobalPiston:
                organInterface->recallGlobalPiston(event.value);
                break;
            case SetGlobalPiston:
                organInterface->setGlobalPiston(event.value);
                break;
            case StopOff:
                organInterface->setDivisionStopOff(event.channel, event.value);
                break;
            case StopOn:
                organInterface->setDivisionStopOn(event.channel, event.value);
                break;
            case StopToggle:
                organInterface->setDivisionStopToggle(event.channel, event.value);
                break;
            case AllDivisionStopsOff:
                organInterface->setDivisionAllStopsOff(event.channel);
                break;
            case AllDivisionStopsOn:
                organInterface->setDivisionAllStopsOn(event.channel);
                break;
            case DivisionCouplerOn:
                organInterface->setDivisionCouplerOn(event.channel, event.value);
                break;
            case DivisionCouplerOff:
                organInterface->setDivisionCouplerOff(event.channel, event.value);
                break;
            case DivisionTremulantOn:
                organInterface->setDivisionTremulantOn(event.channel);
                break;
            case DivisionTremulantOff:
                organInterface->setDivisionTremulantOff(event.channel);
                break;
            default:
                assert(false);
        }
    }

    void handleCC(const MidiData& event) {
        constexpr float DYNAMIC_RANGE_R = 1.0f / 127.0f;
        enum DivisionControl{
            Swell = 7, // CC VOLUME
            AllDivisionNotesOff = 121, // CC RESET
            AllGlobalNotesOff = 123 // CC ALL NOTES OFF
        };
        switch (static_cast<DivisionControl>(event.param)) {
            case Swell:
                organInterface->handleDivisionSwell(event.channel, static_cast<float>(event.value) * DYNAMIC_RANGE_R);
                break;
            case AllDivisionNotesOff:
                organInterface->setDivisionAllNotesOff(event.channel);
            case AllGlobalNotesOff:
                organInterface->setGlobalAllNotesOff();
                break;
        }
    }

    void ProcessMidiEvent(const MidiData& event) {
        switch (event.eventType) {
            case MidiData::NOTE_ON:
                std::cout << "MidiManager - NOTE ON" << std::endl;
                organInterface->setDivisionNoteOn(event.channel, event.param);
                break;
            case MidiData::NOTE_OFF:
                std::cout << "MidiManager - NOTE OFF" << std::endl;
                organInterface->setDivisionNoteOff(event.channel, event.param);
                break;
            case MidiData::CC:
                handleCC(event);
                break;
            case MidiData::PC:
                handlePC(event);
                break;
            default:
                throw std::runtime_error("Unknown Midi Event");
        }
    }
};

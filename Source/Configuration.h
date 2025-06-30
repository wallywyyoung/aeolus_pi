//
// Created by Wally Young on 6/29/25.
//
#pragma once

#include <memory>

#include "aeolus/IR.h"
#include "aeolus/rankwave.h"

class Configuration {
public:
    virtual ~Configuration() = default;
    [[nodiscard]] virtual const int getMIDISwellChannelsMask() const = 0;
    [[nodiscard]] virtual const bool shouldMTSFilterNoteByChannel(int midiNote, int midiChannel) const = 0;
    [[nodiscard]] virtual const bool isMTSEnabled() const = 0;
    [[nodiscard]] virtual const IRs& getIRs() const noexcept = 0;
    [[nodiscard]] virtual const float getMTSNoteToFrequency(int midiNote, int midiChannel) const = 0;
    [[nodiscard]] virtual const std::shared_ptr<Rankwave> getStopByName(const std::string& name) const = 0;
};

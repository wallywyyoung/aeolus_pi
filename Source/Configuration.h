// //
// // Created by Wally Young on 6/29/25.
// //
// #pragma once
//
// #include "aeolus/IR.h"
// #include "aeolus/rankwave.h"
//
// class Configuration {
// public:
//     static Configuration& getInstance() {
//         static Configuration instance;
//         return instance;
//     }
//     virtual ~Configuration() = default;
//     [[nodiscard]] virtual const int getMIDISwellChannelsMask() const = 0;
//     [[nodiscard]] virtual const bool shouldMTSFilterNoteByChannel(int midiNote, int midiChannel) const = 0;
//     [[nodiscard]] virtual const bool isMTSEnabled() const = 0;
//     [[nodiscard]] virtual const IRs& getIRs() const noexcept = 0;
//     [[nodiscard]] virtual const float getMTSNoteToFrequency(int midiNote, int midiChannel) const = 0;
//     [[nodiscard]] virtual Rankwave *getStopByName(const std::string &name) const = 0;
// };

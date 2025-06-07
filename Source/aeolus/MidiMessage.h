//
// Created by Wally Young on 6/6/25.
//

#ifndef AEOLUS_PI_MIDIMESSAGE_H
#define AEOLUS_PI_MIDIMESSAGE_H


struct MidiMessage {

    [[nodiscard]] int getControllerNumber() const {
        return controllerNumber;
    }

    [[nodiscard]] int getControllerValue() const {
        return controllerValue;
    }

    [[nodiscard]] int getChannel() const {
        return channel;
    }

private:
    int controllerNumber, controllerValue, channel;
};


#endif //AEOLUS_PI_MIDIMESSAGE_H

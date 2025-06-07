//
// Created by Wally Young on 6/6/25.
//

#ifndef AEOLUS_PI_IOMANAGER_H
#define AEOLUS_PI_IOMANAGER_H

#include "aeolus/IR.h"

class IOManager {
    void loadOrganDefinition();
    IRs loadIRs();
    void loadExternalPipes();
    void loadEmbeddedPipes();
    void populateDivisions();
    void loadDivisionsFromConfig(std::ifstream& stream);
private:
    void readIRWav(IR ir, std::string filePath);
};


#endif //AEOLUS_PI_IOMANAGER_H

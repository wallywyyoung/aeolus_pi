
#ifndef AEOLUS_PI_RESTMANAGER_H
#define AEOLUS_PI_RESTMANAGER_H


#include "aeolus/Organ.h"

class RestManager {
public:
    static void runServer(std::shared_ptr<Organ> organ);
};


#endif // AEOLUS_PI_RESTMANAGER_H

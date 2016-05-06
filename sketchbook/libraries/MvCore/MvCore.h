#ifndef _MVCORE_H
#define _MVCORE_H

#include "MvStorage.h"
#include "MvFrameHandler.h"

#define VIBRATE_TIMEOUT 100000 // 100ms
#define REC_MIN_TIMEOUT 7*60

/**
 * MvCore
 * @brief Main object that implements the Movuino functionalities
 */
class MvCore {
    public:
        void setup(MvStorage *storage, MvFrameHandler *fhandler,
                   int sens_addr, int pin_button, int pin_vibrate);
        void setupLed(int pin, int logicOn);
        void loop();
};

#endif /* _MVCORE_H */

#ifndef _MVCORE_H
#define _MVCORE_H

#include "MvStorage.h"
#include "MvFrameHandler.h"

/**
 * MvCore
 * @brief Main object that implements the Movuino functionalities
 */
class MvCore {
    public:
        void setup(MvStorage *storage, MvFrameHandler *fhandler,
                   int sens_addr, int pin_button, int pin_led);
        void loop();
};

#endif /* _MVCORE_H */

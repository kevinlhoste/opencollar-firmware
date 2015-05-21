#include "MvFrameHandler.h"
#include "MvCom.h"

class DummyMvCom : public MvCom
{
    public:
        int write_frame(char *frame, int size)
        {
            Serial.print("MvCom write_frame\n");
        }

        int read_frame(char *frame, int *size)
        {
            Serial.print("MvCom read_frame\n");
        }

        void set_mode(enum mvCom_mode mode)
        {
            Serial.print("MvCom set_mode\n");
        }

        enum mvCom_mode get_mode(void)
        {
            Serial.print("MvCom get_mode\n");
        }
};


void setup()
{
    Serial.begin(38400);
}

void loop()
{
}

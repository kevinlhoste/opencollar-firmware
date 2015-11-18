#include "frame_struct.h"

const struct cfg_default cfg_id_list[CFG_ID_LIST_SIZE] = {
    /** accelerometer sensibility */
    {CFG_ID_ACC_SENS, CFG_ACC_SENS_4G},
    /** gyroscope sensibility */
    {CFG_ID_GYRO_SENS,CFG_GYRO_SENS_500DS},
    /** sampling rate */
    {CFG_ID_SAMPLING_RATE, 10},
    /** enable raw accelerometer at live mode */
    {CFG_ID_LIVE_ACC_RAW_EN, CFG_LIVE_ENABLE},
    /** enable raw gyroscope at live mode */
    {CFG_ID_LIVE_GYRO_RAW_EN, CFG_LIVE_ENABLE},
    /** enable raw magnetometer at live mode */
    {CFG_ID_LIVE_MAG_RAW_EN, CFG_LIVE_ENABLE},
    /** enable raw altimeter at live mode */
    {CFG_ID_LIVE_ALT_RAW_EN, CFG_LIVE_ENABLE},
#ifdef MV_ACC_GYRO_DMP_EN
    /** enable quaternion at live mode */
    {CFG_ID_LIVE_QUATERNION_EN, CFG_LIVE_ENABLE},
#endif //#ifdef MV_ACC_GYRO_DMP_EN
};

// NOTE: this is not optmized but save some code size
int cfg_id_get_index(enum cfg_id id)
{
    unsigned int i;

    for (i = 0; i < CFG_ID_LIST_SIZE; i++)
    {
        if (cfg_id_list[i].id == id)
            return i;
    }

    return ANS_NACK_UNKNOWN_CFG;
}

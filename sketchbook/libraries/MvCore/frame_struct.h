#ifndef _FRAME_STRUCT_H
#define _FRAME_STRUCT_H

#include <Arduino.h>

/**
 * cmd_id
 *
 * Used in the struct cmd to specify the value of
 * uint8_t id, which identify the type of the header
 * of the command frame
 */
enum cmd_id {
    /** ping cmd, demands a simple answer from the other device */
    CMD_PING = '?',
    /** asks movuino to enter in live mode */
    CMD_LIVE_START = 'l',
    /** asks movuino to exit the live mode */
    CMD_LIVE_STOP = 'L',
    /** asks movuino to enter in recording mode */
    CMD_REC_START = 'r',
    /** asks movuino to exit the recording mode */
    CMD_REC_STOP = 'R',
    /** asks for recorded data */
    CMD_REC_PLAY = 'p',
    /** ask movuino to clear the records in memory */
    CMD_REC_CLEAR = 'P',
    /** ask for the movuino version */
    CMD_VERSION_GET = 'V',
    /** ask to configure a movuino parameter */
    CMD_CONFIG_SET = 'c',
    /** ask for the movoino configuration */
    CMD_CONFIG_GET = 'C',
    /** switch between ascii and binary mode */
    CMD_SWITCH_MODE = 'm',
    /** Active gpio from the vibrator by 100ms */
    CMD_VIBRATE = 'v'
};

/**
 * cfg_id
 *
 * Used in the struct cfg to specify the value of uint8_t id
 * to identify the subtype of the command frame of configuration
 * @brief sub-command from
 * @see struct cfg
 */
enum cfg_id {
    /** accelerometer sensibility */
    CFG_ID_ACC_SENS = 'A',
    /** gyroscope sensibility */
    CFG_ID_GYRO_SENS = 'G',
    /** sampling rate */
    CFG_ID_SAMPLING_RATE = 's',
    /** enable raw accelerometer at live mode */
    CFG_ID_LIVE_ACC_RAW_EN = 'a',
    /** enable raw gyroscope at live mode */
    CFG_ID_LIVE_GYRO_RAW_EN = 'g',
    /** enable raw magnetometer at live mode */
    CFG_ID_LIVE_MAG_RAW_EN = 'm',
    /** enable altimeter (pressure) at live mode */
    CFG_ID_LIVE_ALT_RAW_EN = 'p',
    /** enable quaternion at live mode */
    CFG_ID_LIVE_QUATERNION_EN = 'q',
};
// NOTE: update this value when changing the cfg_id
#ifdef MV_ACC_GYRO_DMP_EN
    #define CFG_ID_LIST_SIZE 8
#else
    #define CFG_ID_LIST_SIZE 7
#endif

struct cfg_default
{
    enum cfg_id id;
    uint8_t default_val;
};
extern const struct cfg_default cfg_id_list[CFG_ID_LIST_SIZE];
int cfg_id_get_index(enum cfg_id id);

/**
 * cfg_live
 * Possible values to configure the different
 * live mode possibilities
 * @see struct cfg
 */
enum cfg_live {
    CFG_LIVE_DISABLE = 0,
    CFG_LIVE_ENABLE = 1
};

/**
 * cfg_acc_sens
 *
 * Possible values to configure the accelerometer
 * sensibility, used in the struct cfg
 * @see struct cfg
 */
enum cfg_acc_sens {
    /** set max sensibility to 2G */
    CFG_ACC_SENS_2G = 0,
    /** set max sensibility to 4G */
    CFG_ACC_SENS_4G = 1,
    /** set max sensibility to 8G */
    CFG_ACC_SENS_8G = 2,
    /** set max sensibility to 16G */
    CFG_ACC_SENS_16G = 3
};

/**
 * cfg_gyro_sens
 *
 * Possible values to configure the gyroscope
 * sensibility, used in the struct cfg
 * @see struct cfg
 */
enum cfg_gyro_sens {
    /** set max sensibility to 250 degrees/second */
    CFG_GYRO_SENS_250DS = 0,
    /** set max sensibility to 500 degrees/second */
    CFG_GYRO_SENS_500DS = 1,
    /** set max sensibility to 1000 degrees/second */
    CFG_GYRO_SENS_1000DS = 2,
    /** set max sensibility to 2000 degrees/second */
    CFG_GYRO_SENS_2000DS = 3
};

/**
 * cfg
 *
 * Describe the configuration command and answer
 */
struct cfg {
    /** value from enum cfg_id that describes the parameter to be configured */
    uint8_t id;
    /** value that will be used to configure the parameter
     * when id == CFG_ID_ACC_SENS value is take from enum cfg_acc_sens
     * when id == CFG_ID_GYRO_SENS value is take from enum cfg_gyro_sens
     * when id == CFG_ID_SAMPLING_RATE value is the the exact value in the
     *            variable in Hz
     * when id == CFG_ID_LIVE_[*] value is taken from enum cfg_live */
    // NOTE: If you change the type of this variable, the macro CFG_VALUE_VALID_BOUNDARIES must be changed
    uint8_t value;
};
#define CFG_VALUE_VALID_BOUNDARIES(n) (n >= 0 && n <= 255)

/**
 * cmd
 *
 * Describre the received command frame
 * it has the packed attribute, so its data will be contigous to parse
 * it from a binary stream
 */
struct __attribute__((packed)) cmd {
    /** value from enum cmd_id that describes the command type */
    uint8_t id;
    /** store the additional data from the command */
    union {
        /** data from the configuration command
         * @note must not be read or written when the id field is not CFG_SET */
        struct cfg cfg;
    } sub;
};


/**
 * answer_id
 *
 * Used to identify the different types of answer frames
 * at uint8_t id in struct answer
 * @see struct answer
 */
enum answer_id {
    /** answer to succesfull commands */
    ANS_ID_ACK = 'A',
    /** answer to unsuccesfull commands */
    ANS_ID_NACK = 'N',
    /** answer to the version command */
    ANS_ID_VERSION = 'V',
    /** answer to the config command */
    ANS_ID_CONFIG_GET = 'C',
    /** frame header for the live frames */
    ANS_ID_LIVE = 'l',
    /** frame header for the play frames */
    ANS_ID_REC_PLAY = '>'
};

/**
 * ans_nack
 *
 * Identifier of the nack reason
 * @note This value is always a negative number
 * @see struct answer
 */
enum ans_nack {
    /** the received frame is in a bad format */
    ANS_NACK_BAD_FRAME_FORMAT = -1,
    /** the record memory is empty */
    ANS_NACK_EMPTY_MEMORY = -2,
    /** the record memory is full */
    ANS_NACK_MEMORY_FULL = -3,
    /** the flash storage is unavailable */
    ANS_NACK_MEMORY_UNAVAILABLE = -4,
    /** the received frame came with an unknown command */
    ANS_NACK_UNKNOWN_CMD = -5,
    /** the configuration command came with an unknown configuration value */
    ANS_NACK_UNKNOWN_CFG = -6,
    /** internal error, this should never happen */
    ANS_NACK_INTERNAL_ERR = -7
};

/**
 * sensor_type
 *
 * identify the sensor or the modeling of the sensor that is being refered
 * @see struct sensor_data
 */
enum sensor_type {
    /** raw accelerometer */
    SENS_ACC_RAW = 'a',
    /** raw gyroscope */
    SENS_GYRO_RAW = 'g',
    /** raw magnetometer */
    SENS_MAG_RAW = 'm',
    /** raw altimeter (pressure) */
    SENS_ALT_RAW = 'p',
    /** quaternion */
    SENS_QUAT = 'q',
};

/**
 * sensor_3_axes
 *
 * raw data from the 3 axes of the sensor
 * @see struct sensor_data
 */
struct sensor_3_axes {
    uint32_t
        ts;// time stamp
    int16_t
        x,
        y,
        z;
};

/**
 * sensor_single
 *
 * raw data from a single value sensor with 32 bits
 * @see struct sensor_data
 */
struct sensor_single {
    uint32_t
        ts, // time stamp
        p;
};

/**
 * sensor_quaternion
 *
 * data from the quaternion modeling of the sensor data
 * @see struct sensor_data
 */
struct sensor_quaternion {
    // TODO: check how to serialize this
    float
        w,
        x,
        y,
        z;
};

/**
 * sensor_data
 *
 * data extracted from the sensor
 */
struct __attribute__((packed)) sensor_data {
    uint8_t type;
    union {
        struct sensor_3_axes raw;
        struct sensor_quaternion quat;
        struct sensor_single single;
    } data;
};

/**
 * answer
 *
 * constain the information necessary to send a frame to the external user
 * it has the packed attribute, so its data will be contigous to transform
 * it in a binary stream
 */
struct __attribute__((packed)) answer {
    uint8_t id;
    union {
        int8_t nack_value;
        uint8_t version[3];
        struct cfg cfg;
        struct sensor_data sensor_data;
    } sub;
};

#endif /* _FRAME_STRUCT_H */

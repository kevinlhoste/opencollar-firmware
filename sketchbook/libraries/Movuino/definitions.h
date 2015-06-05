#ifndef _DEFINITIONS_H
#define _DEFINITIONS_H

/** 
 * Uncomment the define below to compile with DMP enabled (it will turn quaternions on)
 */
//#define MV_ACC_GYRO_DMP_EN

/**
 * Uncomment the define below to compile eulers code
 */
//#define MV_ACC_GYRO_DMP_EULER_EN

/**
 * Uncomment the define below to compile eulers code
 */
//#define MV_ACC_GYRO_DMP_GRAV_EN

/**
 * If euler or grav is enabled, the DMP should be enabled as well
 */
#if defined(MV_ACC_GYRO_DMP_EULER_EN) || defined(MV_ACC_GYRO_DMP_GRAV_EN)
#define MV_ACC_GYRO_DMP_EN
#endif

/**
 * BUFFER_SIZE
 *
 * Read and write buffer size used in MvFrameHandler class
 */
#define BUFFER_SIZE 100

/**
 * MAX_COM_PORTS
 *
 * Maximum number of com ports supported by MvFrameHandler class
 */
#define MAX_COM_PORTS 2

/**
 * FRAME_ASCII_PREFIX
 *
 * The frame identifier printed in all answers in ascii mode
 */
#define FRAME_ASCII_PREFIX "F: %c "

/**
 * FLOAT_BUFFER_SIZE
 *
 * The max buffer size used to transform a float into a string
 */
#define FLOAT_BUFFER_SIZE 15


#endif /* _DEFINITIONS_H */

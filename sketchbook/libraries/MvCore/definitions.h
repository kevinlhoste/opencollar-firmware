#ifndef _DEFINITIONS_H
#define _DEFINITIONS_H

/** 
 * Uncomment the define below to compile with DMP enabled (it will turn quaternions on)
 */
//#define MV_SENS_DMP_EN

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

#endif /* _DEFINITIONS_H */

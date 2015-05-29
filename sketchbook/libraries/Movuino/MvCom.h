#ifndef _MVCOM_H
#define _MVCOM_H

/**
 * mvCom_mode
 *
 * @brief Identifyier of the comunication mode format
 */
enum mvCom_mode {
    /** Refers to the binary format */
    MVCOM_BINARY,
    /** Refers to the ascii format */
    MVCOM_ASCII
};

/**
 * MvCom
 * @brief Abstract class to be implemented by the communications ports
 */
class MvCom {
    public:
        /**
         * write_frame
         *
         * @brief Send a frame through the COM port
         *
         * @param frame the pointer to the string of bytes to be send
         * @param size  how many bytes to be send
         * @return 0 if success
         */
        virtual int write_frame(char *frame, int size) = 0;

        /**
         * read_frame
         *
         * @brief Read a frame from the COM port if any or NULL will be read
         * @param frame the pointer to the pre-allocated buffer to be filled
         *              by this function with the received frame
         * @param size  this variable will be filled by this function with the
         *              frame size or 0 if no frame is available
         * @return 0 if success
         */
        virtual int read_frame(char *frame, int *size) = 0;

        /**
         * set_mode
         *
         * @brief Change the current communication API mode
         * @param mode The new mode
         * @return 0 if success
         *         ANS_NACK_UNKNOWN_CMD if switch mode is not supported
         */
        virtual int set_mode(enum mvCom_mode mode) = 0;

        /**
         * get_mode
         *
         * @brief Get the current API mode
         * @return the current mode
         * @see enum mvCom_mode
         */
        virtual enum mvCom_mode get_mode(void) = 0;
};

#endif /* _MVCOM_H */

#include "MvFrameHandler.h"

/**
 * MvFrameHandler constructor
 *
 * @brief initialize the MvFrameHandler object
 *
 * @param com_list  the list of the MvCom objects to work with
 * @param size      the size of the com_list
 */
MvFrameHandler::MvFrameHandler(MvCom **com_list, int size)
{
    this->com_list_size = size;
    for (int i = 0; i < size; i++)
    {
        this->com_list[i] = com_list[i];
    }
}

/**
 * ignore_spaces
 *
 * @brief Return the index in the buffer that is different from space
 *          or tab, starting from the index in te variable start.
 *          If none, then the size of the buffer is returned.
 *
 * @param buffer    the buffer to be analized
 * @param size      the max size of the buffer
 * @param start     the index to start analizing the buffer
 *
 * @return the index pointing to a character different from space or tab,
 *          or the size of the buffer if none.
 */
int MvFrameHandler::ignore_spaces(char *buffer, int size, int start)
{
    int i;

    // ignore the spaces at the begining
    for(i = start; i < size; i++)
    {
        if(buffer[i] != ' ' && buffer[i] != '\t')
            break;
    }

    return i;
}

/**
 * parse_cmd_frame_bin_mode
 *
 * @brief The same as parse_cmd_frame but specific to the mode binary
 *
 * @see parse_cmd_frame
 */
int MvFrameHandler::parse_cmd_frame_bin_mode(char *read_buffer, int read_size,
                                                struct cmd *cmd)
{
    // TODO
}

/**
 * parse_cmd_frame_ascii_mode
 *
 * @brief The same as parse_cmd_frame but specific to the mode ascii
 *
 * @see parse_cmd_frame
 */
int MvFrameHandler::parse_cmd_frame_ascii_mode(char *read_buffer, int read_size,
                                                struct cmd *cmd)
{
    int i;

    i = this->ignore_spaces(read_buffer, read_size, 0);
    if (i == read_size)
        return ERR_BAD_FRAME;

    // Read the command id
    cmd->id = read_buffer[i++];

    // Read the others parameters
    if(cmd->id == CMD_CONFIG_SET)
    {
        i = this->ignore_spaces(read_buffer, read_size, i);
        if (i == read_size)
            return ERR_BAD_FRAME;

        cmd->sub.cfg.id = read_buffer[i++];

        i = this->ignore_spaces(read_buffer, read_size, i);
        if (i == read_size)
            return ERR_BAD_FRAME;

        // NOTE: this is not that good, but I am assuming that size of the buffer
        // will always be bigger then the buffer size and if it is exactly equal its
        // little corner case bug that will almost never happen
        if(read_size < BUFFER_SIZE)
            read_buffer[read_size] = '\0';
        else
            return ERR_BAD_FRAME;

        // TODO: return error if not a number
        String tmp = String(&read_buffer[i]);
        cmd->sub.cfg.value = tmp.toInt();
    }

    return SUCCESS_FRAME_READ;
}

/**
 * parse_cmd_frame
 *
 * @brief parse the string of bytes read_buffer and fill the cmd structure
 *          according with the mode
 *
 * @param read_buffer   the buffer to be parsed
 * @param read_size     the size of the buffer to be parsed
 * @param cmd           the struct cmd to be filled
 * @param mode          the mode to interpret the buffer
 *
 * @return  SUCCESS_FRAME_READ if a frame was read and successfuly parsed
 *          ERR_BAD_FRAME if a frame could not be parsed
 *          ERR_BAD_PARAM if read_buffer is a null pointer or read_size is zero
 */
int MvFrameHandler::parse_cmd_frame(char *read_buffer, int read_size,
                                    struct cmd *cmd, enum mvCom_mode mode)
{
    if(!read_buffer || !read_size)
        return ERR_BAD_PARAM;

    switch(mode)
    {
        case MVCOM_ASCII:
            return this->parse_cmd_frame_ascii_mode(read_buffer, read_size, cmd);
        case MVCOM_BINARY:
            return this->parse_cmd_frame_bin_mode(read_buffer, read_size, cmd);
    }
}

/**
 * build_answer_frame_ascii_mode
 *
 * @brief The same as build_answer_frame but specific to the mode ascii
 *
 * @see build_answer_frame
 */
int MvFrameHandler::build_answer_frame_ascii_mode(char *buffer, struct answer *ans)
{
    switch(ans->id)
    {
        case ANS_ID_ACK:
            sprintf(buffer, FRAME_ASCII_PREFIX "OK\n", ans->id);
            goto ret;

        case ANS_ID_NACK:
            // TODO: interpret nack values and print the not just the error number
            // but the error name in ascii too
            sprintf(buffer, FRAME_ASCII_PREFIX "ERR:%d\n", ans->id, ans->sub.nack_value);
            goto ret;

        case ANS_ID_VERSION:
            sprintf(buffer, FRAME_ASCII_PREFIX "VERSION:%d.%d.%d\n",
                    ans->id,
                    ans->sub.version[0],
                    ans->sub.version[1],
                    ans->sub.version[2]);
            goto ret;

        case ANS_ID_CONFIG_GET:
            sprintf(buffer, FRAME_ASCII_PREFIX "CFG:%c VAL:%d\n",
                    ans->id,
                    ans->sub.cfg.id,
                    ans->sub.cfg.value);
            goto ret;

        case ANS_ID_LIVE:
        case ANS_ID_REC_PLAY:
            switch(ans->sub.sensor_data.type)
            {
                char float_str[4][FLOAT_BUFFER_SIZE];

                case SENS_ACC_RAW:
                case SENS_GYRO_RAW:
                    sprintf(buffer, FRAME_ASCII_PREFIX "SENSOR:%c X:%d Y:%d Z:%d\n",
                            ans->id,
                            ans->sub.sensor_data.type,
                            ans->sub.sensor_data.data.raw.x,
                            ans->sub.sensor_data.data.raw.y,
                            ans->sub.sensor_data.data.raw.z);
                    goto ret;

                case SENS_QUAT:
                    // Float to string
                    dtostrf(ans->sub.sensor_data.data.quat.w, 1, 2, float_str[0]);
                    dtostrf(ans->sub.sensor_data.data.quat.x, 1, 2, float_str[1]);
                    dtostrf(ans->sub.sensor_data.data.quat.y, 1, 2, float_str[2]);
                    dtostrf(ans->sub.sensor_data.data.quat.z, 1, 2, float_str[3]);

                    sprintf(buffer, FRAME_ASCII_PREFIX "SENSOR:%c W:%s X:%s Y:%s Z:%s\n",
                            ans->id,
                            ans->sub.sensor_data.type,
                            float_str[0],
                            float_str[1],
                            float_str[2],
                            float_str[3]);
                    goto ret;

                case SENS_EULER:
                    // Float to string
                    dtostrf(ans->sub.sensor_data.data.euler.psi,     1, 2, float_str[0]);
                    dtostrf(ans->sub.sensor_data.data.euler.theta,   1, 2, float_str[1]);
                    dtostrf(ans->sub.sensor_data.data.euler.phi,     1, 2, float_str[2]);

                    sprintf(buffer, FRAME_ASCII_PREFIX "SENSOR:%c PSI:%s THETA:%s PHI:%s\n",
                            ans->id,
                            ans->sub.sensor_data.type,
                            float_str[0],
                            float_str[1],
                            float_str[2]);
                    goto ret;

                case SENS_GRAVITY:
                    // Float to string
                    dtostrf(ans->sub.sensor_data.data.gravity.yaw,     1, 2, float_str[0]);
                    dtostrf(ans->sub.sensor_data.data.gravity.pitch,   1, 2, float_str[1]);
                    dtostrf(ans->sub.sensor_data.data.gravity.roll,    1, 2, float_str[2]);

                    sprintf(buffer, FRAME_ASCII_PREFIX "SENSOR:%c YAW:%s PITCH:%s ROLL:%s\n",
                            ans->id,
                            ans->sub.sensor_data.type,
                            float_str[0],
                            float_str[1],
                            float_str[2]);
                    goto ret;
            }
    }

ret:
    return strlen(buffer);
}

/**
 * build_answer_frame_bin_mode
 *
 * @brief The same as build_answer_frame but specific to the mode ascii
 *
 * @see build_answer_frame
 */
int MvFrameHandler::build_answer_frame_bin_mode(char *buffer, struct answer *ans)
{
    // TODO
}

/**
 * build_answer_frame
 *
 * @brief Interprete the struct answer and fill the buffer with the serialized info
 *          according with the mode
 *
 * @param buffer    the buffer to be filled
 * @param ans       the struct answer to be interpreted
 * @param mode      the mode to fill the buffer
 *
 * @return the size of the buffer or a negative number error from
 *          enum return_msg
 */
int MvFrameHandler::build_answer_frame(char *buffer, struct answer *ans,
                                        enum mvCom_mode mode)
{
    if(!buffer || !ans)
        return ERR_BAD_PARAM;

    switch(mode)
    {
        case MVCOM_ASCII:
            return this->build_answer_frame_ascii_mode(buffer, ans);
        case MVCOM_BINARY:
            return this->build_answer_frame_bin_mode(buffer, ans);
    }
}

/**
 * write_frame
 *
 * @brief send the answer of a command. The frame->answer must be pre-filled
 *          by the user
 *
 * @param frame The frame containing information of what must be send
 *
 * @return  0 if success
 *          ERR_BAD_FRAME if an error has occured while parsing the answer structure
 *          ERR_BAD_PARAM if struct frame *frame is a NULL pointer
 */
int MvFrameHandler::write_frame(struct frame *frame)
{
    if(!frame) return ERR_BAD_PARAM;

    enum mvCom_mode mode = (*this->com_list[frame->com]).get_mode();

    int ret = this->build_answer_frame(buffer, &(frame->answer), mode);

    if(ret > 0)
    {
        (*this->com_list[frame->com]).write_frame(buffer, ret);
    }

    return ret;
}

/**
 * read_frame
 *
 * @brief try to read a frame from one of the com ports
 *
 * @param frame the struct frame to be filled if a read is successful
 *
 * @return  SUCCESS_FRAME_READ if a frame was read and successfuly parsed
 *          SUCCESS_NO_FRAME_WAS_READ if no frame was read
 *          ERR_BAD_FRAME if a frame was read but it could not be parsed
 *          ERR_BAD_PARAM if struct frame *frame is a NULL pointer
 */
int MvFrameHandler::read_frame(struct frame *frame)
{
    if(!frame) return ERR_BAD_PARAM;

    int read_size = 0;

    // Iterate through all the com ports
    for(int i = 0; i < this->com_list_size; i++)
    {
        (*this->com_list[i]).read_frame(buffer, &read_size);
        if(read_size)
        {
            // Save the com port that we received the frame
            frame->com = i;

            return this->parse_cmd_frame(buffer, read_size, &frame->cmd,
                                            (*this->com_list[i]).get_mode());
        }
    }

    return SUCCESS_NO_FRAME_WAS_READ;
}

int MvFrameHandler::exec_com_cmd()
{
    // TODO
}

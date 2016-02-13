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
    *cmd = *(struct cmd*)read_buffer;

    /* Check size error */

    /* The config set command has the size of the id plus the size of its arguments */
    /* All the other commands has no arguments */
    if ((cmd->id == CMD_CONFIG_SET && read_size != sizeof(cmd->id) + sizeof(cmd->sub.cfg)) ||
        (cmd->id != CMD_CONFIG_SET && read_size != sizeof(cmd->id)))
        return ANS_NACK_BAD_FRAME_FORMAT;

    return SUCCESS_FRAME_READ;
}

static bool isDigit(char c)
{
    if (c < '0' || c > '9')
        return false;
    return true;
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
        return ANS_NACK_BAD_FRAME_FORMAT;

    // Read the command id
    cmd->id = read_buffer[i++];

    // Read the others parameters
    if(cmd->id == CMD_CONFIG_SET)
    {
        String intStr = "";
        int number;

        i = this->ignore_spaces(read_buffer, read_size, i);
        if (i == read_size)
            return ANS_NACK_BAD_FRAME_FORMAT;

        cmd->sub.cfg.id = read_buffer[i++];

        i = this->ignore_spaces(read_buffer, read_size, i);
        if (i == read_size)
            return ANS_NACK_BAD_FRAME_FORMAT;

        // Check if we have at least one digit
        // NOTE: the minus sign is being considered a digit, if there
        // is no other digit after the minus sign it is interpreted as zero
        if(!isDigit((char)read_buffer[i]) && (char)read_buffer[i] !='-')
            return ANS_NACK_BAD_FRAME_FORMAT;

        // Read all the other digits into intStr
        do {
            intStr += (char)read_buffer[i++];
        } while(i < read_size && isDigit(read_buffer[i]));

        // Check number boundaries to fit in the cfg.value field
        number = intStr.toInt();
        if (!CFG_VALUE_VALID_BOUNDARIES(number))
            return ANS_NACK_BAD_FRAME_FORMAT;

        cmd->sub.cfg.value = number;
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
 *          ANS_NACK_BAD_FRAME_FORMAT if a frame could not be parsed
 *          ANS_NACK_INTERNAL_ERR if read_buffer is a null pointer or read_size is zero
 */
int MvFrameHandler::parse_cmd_frame(char *read_buffer, int read_size,
                                    struct cmd *cmd, enum mvCom_mode mode)
{
    if(!read_buffer || !read_size)
        return ANS_NACK_INTERNAL_ERR;

    switch(mode)
    {
        case MVCOM_ASCII:
            return this->parse_cmd_frame_ascii_mode(read_buffer, read_size, cmd);
        case MVCOM_BINARY:
            return this->parse_cmd_frame_bin_mode(read_buffer, read_size, cmd);
    }
}

/**
 * err_description
 *
 * @brief Get the string with the description of the error
 *
 * @param nack_value the error id from enum ans_nack
 *
 * @return the pointer to the string with the description
 */
const char * MvFrameHandler::err_description(int nack_value)
{
    switch(nack_value)
    {
        case ANS_NACK_BAD_FRAME_FORMAT:
            return "bad frame format";
        case ANS_NACK_EMPTY_MEMORY:
            return "memory is empty";
        case ANS_NACK_MEMORY_FULL:
            return "memory is full";
        case ANS_NACK_UNKNOWN_CMD:
            return "unknown command";
        case ANS_NACK_UNKNOWN_CFG:
            return "unknown configuration";
        case ANS_NACK_INTERNAL_ERR:
            return "internal error";
        default:
            return "unknown error";
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
            sprintf(buffer, FRAME_ASCII_PREFIX "DESC: OK", ans->id);
            goto ret;

        case ANS_ID_NACK:
            // TODO: interpret nack values and print the not just the error number
            // but the error name in ascii too
            sprintf(buffer, FRAME_ASCII_PREFIX "ERR: %d DESC: %s", ans->id,
                    ans->sub.nack_value, this->err_description(ans->sub.nack_value));
            goto ret;

        case ANS_ID_VERSION:
            sprintf(buffer, FRAME_ASCII_PREFIX "VER: %d.%d.%d",
                    ans->id,
                    ans->sub.version[0],
                    ans->sub.version[1],
                    ans->sub.version[2]);
            goto ret;

        case ANS_ID_CONFIG_GET:
            sprintf(buffer, FRAME_ASCII_PREFIX "CFG: %c VAL: %d",
                    ans->id,
                    ans->sub.cfg.id,
                    ans->sub.cfg.value);
            goto ret;

        case ANS_ID_LIVE:
        case ANS_ID_REC_PLAY:
            switch(ans->sub.sensor_data.type)
            {
                case SENS_ACC_RAW:
                case SENS_GYRO_RAW:
                case SENS_MAG_RAW:
                    sprintf(buffer, FRAME_ASCII_PREFIX "%c %lu %d %d %d",
                            ans->id,
                            ans->sub.sensor_data.type,
                            ans->sub.sensor_data.data.raw.ts,
                            ans->sub.sensor_data.data.raw.x,
                            ans->sub.sensor_data.data.raw.y,
                            ans->sub.sensor_data.data.raw.z);
                    goto ret;

                case SENS_ALT_RAW:
                    sprintf(buffer, FRAME_ASCII_PREFIX "%c %lu %d",
                            ans->id,
                            ans->sub.sensor_data.type,
                            ans->sub.sensor_data.data.single.ts,
                            ans->sub.sensor_data.data.single.p);
                    goto ret;

                case SENS_QUAT:
                    sprintf(buffer, FRAME_ASCII_PREFIX "SS: %c %.2f %.2f %.2f %.2f",
                            ans->id,
                            ans->sub.sensor_data.type,
                            ans->sub.sensor_data.data.quat.w,
                            ans->sub.sensor_data.data.quat.x,
                            ans->sub.sensor_data.data.quat.y,
                            ans->sub.sensor_data.data.quat.z);
                    goto ret;
            }
    }

ret:
    return strlen(buffer);
}

/**
 * ans_frame_size
 * @brief return the amount of bytes from ans that is filled with data
 */
int MvFrameHandler::ans_frame_size(struct answer *ans)
{
    int size;

    /* The id */
    size = sizeof(ans->id);

    switch(ans->id)
    {
        case ANS_ID_ACK:
            /* This frame has just an id */
            break;

        case ANS_ID_NACK:
            size += sizeof(ans->sub.nack_value);
            break;

        case ANS_ID_VERSION:
            size += sizeof(ans->sub.version);
            break;

        case ANS_ID_CONFIG_GET:
            size += sizeof(ans->sub.cfg);
            break;

        case ANS_ID_LIVE:
        case ANS_ID_REC_PLAY:
            /* The first byte is the live id */
            size += sizeof(ans->sub.sensor_data.type);
            switch(ans->sub.sensor_data.type)
            {
                case SENS_ACC_RAW:
                case SENS_GYRO_RAW:
                case SENS_MAG_RAW:
                    size += sizeof(ans->sub.sensor_data.data.raw);
                    break;
                case SENS_ALT_RAW:
                    size += sizeof(ans->sub.sensor_data.data.single);
                    break;
                case SENS_QUAT:
                    size += sizeof(ans->sub.sensor_data.data.quat);
                    break;
            }
            break;
    }

    return size;
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
    unsigned int size, i;

    size = this->ans_frame_size(ans);
    /* Copy the data to the buffer */
    for (i = 0; i < size; i++)
        buffer[i] = ((char*)ans)[i];

    return size;
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
        return ANS_NACK_INTERNAL_ERR;

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
 * @return  The size of the frame if success
 *          ANS_NACK_BAD_FRAME_FORMAT if an error has occured while parsing the answer structure
 *          ANS_NACK_INTERNAL_ERR if struct frame *frame is a NULL pointer
 */
int MvFrameHandler::write_frame(struct frame *frame)
{
    if(!frame) return ANS_NACK_INTERNAL_ERR;

    enum mvCom_mode mode = frame->com->get_mode();

    int ret = this->build_answer_frame(buffer, &(frame->answer), mode);

    if(ret > 0)
    {
        frame->com->write_frame(buffer, ret);
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
 *          ANS_NACK_BAD_FRAME_FORMAT if a frame was read but it could not be parsed
 *          ANS_NACK_INTERNAL_ERR if struct frame *frame is a NULL pointer
 */
int MvFrameHandler::read_frame(struct frame *frame)
{
    if(!frame) return ANS_NACK_INTERNAL_ERR;

    int read_size = 0;

    // Iterate through all the com ports
    for(int i = 0; i < this->com_list_size; i++)
    {
        this->com_list[i]->read_frame(buffer, &read_size);
        if(read_size)
        {
            // Save the com port that we received the frame
            frame->com = this->com_list[i];

            return this->parse_cmd_frame(buffer, read_size, &frame->cmd,
                                            this->com_list[i]->get_mode());
        }
    }

    return SUCCESS_NO_FRAME_WAS_READ;
}

int MvFrameHandler::parse_ans_frame_bin_mode(char *buffer, int read_size, struct answer *ans)
{
    int size, i;

    size = this->ans_frame_size((struct answer*)buffer);
    if(size != read_size)
        return ERROR_BAD_FRAME_SIZE;

    for(i = 0; i < read_size; i++)
    {
        ((char*)ans)[i] = buffer[i];
    }

    return SUCCESS_FRAME_READ;
}

int MvFrameHandler::parse_ans_frame(char *buffer, int read_size, struct answer *ans, enum mvCom_mode mode)
{
    if(!buffer || !read_size)
        return ANS_NACK_INTERNAL_ERR;

    switch(mode)
    {
        case MVCOM_ASCII:
            /*unsuported*/
            return ANS_NACK_INTERNAL_ERR;
        case MVCOM_BINARY:
            return this->parse_ans_frame_bin_mode(buffer, read_size, ans);
    }
}

int MvFrameHandler::read_answer_frame(struct frame *frame, MvCom *com)
{
    if(!frame) return ANS_NACK_INTERNAL_ERR;

    int read_size = 0;
    if(com->read_frame(buffer, &read_size) < 0)
        return SUCCESS_NO_FRAME_WAS_READ;
    return this->parse_ans_frame(buffer, read_size, &frame->answer, com->get_mode());
}

/**
 * exec_com_cmd
 *
 * @brief Execute a command destinated to a COM port
 *
 * @param frame the frame containing the command to be executed
 *
 * @return  0 if success
 *          ANS_NACK_INTERNAL_ERR if frame is NULL or the command is not
 *          a COM port command
 *
 * @note If the frame->cmd.id command is a CMD_SWITCH_MODE, this functions
 *          changes the mode imediatly. If the answer must be send in the
 *          same mode as the command, this functions must be called
 *          after sending the answer.
 */
int MvFrameHandler::exec_com_cmd(struct frame *frame)
{
    // CMD_SWITCH_MODE is (for now) the only command that this
    // function executes. In the future we can think of other commands
    // as changing the com baudrate for example.
    if(!frame || frame->cmd.id != CMD_SWITCH_MODE)
        return ANS_NACK_INTERNAL_ERR;

    if(frame->com->get_mode() == MVCOM_ASCII)
        return frame->com->set_mode(MVCOM_BINARY);
    else
        return frame->com->set_mode(MVCOM_ASCII);

    return 0;
}

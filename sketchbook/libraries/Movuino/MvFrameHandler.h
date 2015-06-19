#ifndef _MV_FRAME_HANDLER_H
#define _MV_FRAME_HANDLER_H

#include "MvCom.h"
#include "frame_struct.h"
#include "definitions.h"

// Note: most part of the documentation is written in the cpp file

#define SUCCESS_NO_FRAME_WAS_READ 0
#define SUCCESS_FRAME_READ 1
#define ERROR_BAD_FRAME_SIZE -1

/**
 * struct frame
 *
 * @brief The struct frame used in this API, it keeps track of the command and the COM
 *          port to send the answer.
 *          The user must receive this struct using the read_frame function and
 *          interpret the cmd field. Then it must fill the answer field and use
 *          the write_frame function to send the answer to who sent the command.
 */
struct frame {
    /** The cmd field is filled by the read_frame function and interpreted by the user */
    struct cmd cmd;
    /** The cmd field is filled by user of this library */
    struct answer answer;
    /** The MvCom from were the command came from and to were the answer must be sent */
    MvCom *com;
};

class MvFrameHandler {
    public:
        MvFrameHandler(MvCom **com_list, int size);

        int write_frame(struct frame *frame);

        int read_frame(struct frame *frame);

        int exec_com_cmd(struct frame *frame);

        int read_answer_frame(struct frame *frame, MvCom *com);

    private:
        // ----------- variables --------------------------
        /** buffer
         *
         * @brief The buffer to read and write from/to the com ports
         */
        char buffer[BUFFER_SIZE];

        /** com_list
         * @brief The list of the com ports to manage
         */
        MvCom *com_list[MAX_COM_PORTS];

        /** com_list_size
         * @brief The size of the com_list variable
         */
        int com_list_size;

        // ----------- functions --------------------------
        int parse_cmd_frame(char *read_buffer, int read_size,
                            struct cmd *cmd, enum mvCom_mode mode);
        int parse_cmd_frame_ascii_mode(char *read_buffer, int read_size, struct cmd *cmd);
        int parse_cmd_frame_bin_mode(char *read_buffer, int read_size, struct cmd *cmd);
        int build_answer_frame(char *buffer, struct answer *ans, enum mvCom_mode mode);
        int build_answer_frame_ascii_mode(char *buffer, struct answer *ans);
        int build_answer_frame_bin_mode(char *buffer, struct answer *ans);
        int ignore_spaces(char *buffer, int size, int start);
        const char * err_description(int nack_value);
        int ans_frame_size(struct answer *ans);
        int parse_ans_frame_bin_mode(char *buffer, int read_size, struct answer *ans);
        int parse_ans_frame(char *buffer, int read_size, struct answer *ans, enum mvCom_mode mode);

};

#endif /* _MV_FRAME_HANDLER_H */

#ifndef _MV_FRAME_HANDLER_H
#define _MV_FRAME_HANDLER_H

#include "MvCom.h"
#include "frame_struct.h"
#include "definitions.h"

struct frame {
    struct cmd cmd;
    struct answer answer;
    int com;
};

class MvFrameHandler {
    private:
        char buffer[BUFFER_SIZE];
        MvCom *com_list[MAX_COM_PORTS];
        int com_list_size;

    public:
        /**
         * MvFrameHandler constructor
         *
         * @brief initialize the MvFrameHandler object
         *
         * @param com_list  the list of the MvCom objects to work with
         * @param size      the size of the com_list
         */
        MvFrameHandler(MvCom **com_list, int size);

        int write_frame(struct frame *frame);

        int read_frame(struct frame *frame);

        // TODO: define this exec function better
        int exec_com_cmd();
};

#endif /* _MV_FRAME_HANDLER_H */

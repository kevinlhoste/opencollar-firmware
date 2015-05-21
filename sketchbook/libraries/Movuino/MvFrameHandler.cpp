#include "MvFrameHandler.h"

MvFrameHandler::MvFrameHandler(MvCom **com_list, int size)
{
    this->com_list_size = size;
    for (int i = 0; i < size; i++)
    {
        this->com_list[i] = com_list[i];
    }
}

int parse_cmd_frame(char *read_buffer, int read_size, struct cmd *cmd)
{
    // TODO
    return -1;
}

int build_answer_frame(char *buffer, struct answer *ans)
{
    // TODO
    return 0;
}


int MvFrameHandler::write_frame(struct frame *frame)
{
    int ret = build_answer_frame(buffer, &(frame->answer));

    if(ret > 0)
    {
        (*this->com_list[frame->com]).write_frame(buffer, ret);
    }

    return ret;
}

int MvFrameHandler::read_frame(struct frame *frame)
{
    // Iterate through all the com ports
    int read_size = -1;

    for(int i = 0; i < this->com_list_size; i++)
    {
        (*this->com_list[i]).read_frame(buffer, &read_size);
        if(read_size)
        {
            return parse_cmd_frame(buffer, read_size, &frame->cmd);
        }
    }

    return NULL;
}

int MvFrameHandler::exec_com_cmd()
{
    // TODO
}

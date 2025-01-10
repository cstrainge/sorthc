
#include "sorth-runtime.h"
#include "runtime-words.h"
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <sys/socket.h>



using namespace sorth::run_time::data_structures;



extern "C"
{


    uint8_t word_posix_open()
    {
        int64_t flags;
        Value path_value;

        auto pop_result_1 = stack_pop_int(&flags);
        auto pop_result_2 = stack_pop(&path_value);

        if (pop_result_1 || pop_result_2 || !path_value.is_string())
        {
            return 1;
        }

        int fd = open(path_value.get_string().c_str(), flags, 0);

        stack_push_int(fd);

        return 0;
    }


    uint8_t word_posix_errno()
    {
        stack_push_int(errno);

        return 0;
    }


    uint8_t word_posix_set_errno()
    {
        int64_t value;

        auto pop_result = stack_pop_int(&value);

        if (pop_result)
        {
            return 1;
        }

        errno = value;

        return 0;
    }


    uint8_t word_posix_strerror()
    {
        stack_push_string(strerror(errno));

        return 0;
    }


    uint8_t word_posix_fcntl()
    {
        int64_t fd;
        int64_t cmd;

        int result = -1;

        auto pop_result_1 = stack_pop_int(&cmd);
        auto pop_result_2 = stack_pop_int(&fd);

        if (pop_result_1 || pop_result_2)
        {
            return 1;
        }

        stack_push_int(fcntl(fd, cmd));

        return 0;
    }


}


namespace sorth::run_time::abi::words
{


    void register_posix_words(const RuntimeWordRegistrar& registrar)
    {
        registrar("posix.open", "word_posix_open");
        registrar("posix.errno", "word_posix_errno");
        registrar("posix.set-errno", "word_posix_set_errno");
        registrar("posix.strerror", "word_posix_strerror");
        registrar("posix.fcntl", "word_posix_fcntl");
    }


}

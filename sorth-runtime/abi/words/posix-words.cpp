
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


    uint8_t word_posix_get_errno()
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


}


namespace sorth::run_time::abi::words
{


    void register_posix_words(const RuntimeWordRegistrar& registrar)
    {
        registrar("posix.errno@", "word_posix_get_errno");
        registrar("posix.errno!", "word_posix_set_errno");
    }


}

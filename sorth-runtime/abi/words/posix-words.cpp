
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


    uint8_t word_posix_read_buffer()
    {
        int64_t fd;
        Value buffer_value;

        auto pop_result_1 = stack_pop_int(&fd);
        auto pop_result_2 = stack_pop(&buffer_value);

        if (pop_result_1 || pop_result_2)
        {
            return 1;
        }

        if (!buffer_value.is_byte_buffer())
        {
            set_last_error("Expected a byte buffer value.");
            return 1;
        }

        auto buffer = buffer_value.get_byte_buffer();
        auto total_size = buffer->size();

        ssize_t result = 0;

        buffer->set_position(0);

        do
        {
            errno = 0;
            result = read(fd, buffer->position_ptr(), total_size - buffer->position());

            if (result > 0)
            {
                buffer->increment_position(result);
            }
            else if ((result == -1) && (errno != EINTR))
            {
                break;
            }
        }
        while (buffer->position() < total_size);

        stack_push_int(result);

        return 0;
    }


    uint8_t word_posix_write_buffer()
    {
        int64_t fd;
        Value buffer_value;

        auto pop_result_1 = stack_pop_int(&fd);
        auto pop_result_2 = stack_pop(&buffer_value);

        if (pop_result_1 || pop_result_2)
        {
            return 1;
        }

        if (!buffer_value.is_byte_buffer())
        {
            set_last_error("Expected a byte buffer value.");
            return 1;
        }

        auto buffer = buffer_value.get_byte_buffer();
        auto total_size = buffer->size();

        ssize_t result = 0;

        buffer->set_position(0);

        do
        {
            errno = 0;
            result = write(fd, buffer->position_ptr(), total_size - buffer->position());

            if (result > 0)
            {
                buffer->increment_position(result);
            }
            else if ((result == -1) && (errno != EINTR))
            {
                break;
            }
        }
        while (buffer->position() < total_size);

        stack_push_int(result);

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
        registrar("posix.read-buffer", "word_posix_read_buffer");
        registrar("posix.write-buffer", "word_posix_write_buffer");
    }


}

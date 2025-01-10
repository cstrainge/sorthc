
#include "sorth-runtime.h"
#include "byte-buffer-words.h"



using namespace sorth::run_time::data_structures;


namespace
{


    ByteBufferPtr stack_pop_as_byte_buffer()
    {
        Value value;

        auto pop_result = stack_pop(&value);

        if (pop_result)
        {
            return nullptr;
        }

        if (!value.is_byte_buffer())
        {
            set_last_error("Expected a byte buffer value.");
            return nullptr;
        }

        return value.get_byte_buffer();
    }


    uint8_t check_buffer_index(size_t byte_size, const ByteBufferPtr& buffer)
    {
        if (buffer->position() + byte_size > buffer->size())
        {
            std::stringstream stream;

            stream << "Index " << buffer->position()
                   << " with access size " << byte_size
                   << " is out of bounds for buffer size "
                   << buffer->size() << ".";

            set_last_error(stream.str().c_str());

            return 1;
        }

        return 0;
    }


}


extern "C"
{


    uint8_t word_buffer_new()
    {
        int64_t size;

        auto pop_result = stack_pop_int(&size);

        if (pop_result)
        {
            return 1;
        }

        Value buffer = std::make_shared<ByteBuffer>(static_cast<size_t>(size));

        stack_push(&buffer);

        return 0;
    }


    uint8_t word_buffer_write_int()
    {
        int64_t size;
        ByteBufferPtr buffer;
        int64_t value;

        auto pop_result_1 = stack_pop_int(&value);
        buffer = stack_pop_as_byte_buffer();
        auto pop_result_2 = stack_pop_int(&size);

        if (pop_result_1 || pop_result_2 || !buffer)
        {
            return 1;
        }

        if (check_buffer_index(size, buffer))
        {
            return 1;
        }

        buffer->write_int(size, value);

        return 0;
    }


    uint8_t word_buffer_read_int()
    {
        bool is_signed;
        int64_t size;
        ByteBufferPtr buffer;

        auto pop_result_1 = stack_pop_bool(&is_signed);
        auto pop_result_2 = stack_pop_int(&size);
        buffer = stack_pop_as_byte_buffer();

        if (pop_result_1 || pop_result_2 || !buffer)
        {
            return 1;
        }

        if (check_buffer_index(size, buffer))
        {
            return 1;
        }

        stack_push_int(buffer->read_int(size, true));

        return 0;
    }


    uint8_t word_buffer_write_float()
    {
        int64_t size;
        ByteBufferPtr buffer;
        double value;

        auto pop_result_1 = stack_pop_int(&size);
        buffer = stack_pop_as_byte_buffer();
        auto pop_result_2 = stack_pop_double(&value);

        if (pop_result_1 || pop_result_2 || !buffer)
        {
            return 1;
        }

        if (check_buffer_index(size, buffer))
        {
            return 1;
        }

        buffer->write_float(size, value);

        return 0;
    }


    uint8_t word_buffer_read_float()
    {
        int64_t size;
        ByteBufferPtr buffer;

        auto pop_result_1 = stack_pop_int(&size);
        buffer = stack_pop_as_byte_buffer();

        if (pop_result_1 || !buffer)
        {
            return 1;
        }

        if (check_buffer_index(size, buffer))
        {
            return 1;
        }

        stack_push_double(buffer->read_float(size));

        return 0;
    }


    uint8_t word_buffer_write_string()
    {
        int64_t max_size;
        ByteBufferPtr buffer;
        Value value;

        auto pop_result_1 = stack_pop_int(&max_size);
        buffer = stack_pop_as_byte_buffer();
        auto pop_result_2 = stack_pop(&value);

        if (pop_result_1 || pop_result_2 || !buffer || !value.is_string())
        {
            return 1;
        }

        if (check_buffer_index(max_size, buffer))
        {
            return 1;
        }

        buffer->write_string(value.get_string(), max_size);

        return 0;
    }


    uint8_t word_buffer_read_string()
    {
        int64_t max_size;
        ByteBufferPtr buffer;

        auto pop_result_1 = stack_pop_int(&max_size);
        buffer = stack_pop_as_byte_buffer();

        if (pop_result_1 || !buffer)
        {
            return 1;
        }

        if (check_buffer_index(max_size, buffer))
        {
            return 1;
        }

        stack_push_string(buffer->read_string(max_size).c_str());

        return 0;
    }


    uint8_t word_buffer_get_size()
    {
        ByteBufferPtr buffer = stack_pop_as_byte_buffer();

        if (!buffer)
        {
            return 1;
        }

        stack_push_int(buffer->size());

        return 0;
    }


    uint8_t word_buffer_set_position()
    {
        int64_t new_position;
        ByteBufferPtr buffer;

        auto pop_result_1 = stack_pop_int(&new_position);
        buffer = stack_pop_as_byte_buffer();

        if (pop_result_1 || !buffer)
        {
            return 1;
        }

        if (new_position > buffer->size())
        {
            std::stringstream stream;

            stream << "Position " << new_position << " out of bounds for buffer size "
                   << buffer->size() << ".";

            set_last_error(stream.str().c_str());

            return 1;
        }

        buffer->set_position(new_position);

        return 0;
    }


    uint8_t word_buffer_get_position()
    {
        ByteBufferPtr buffer = stack_pop_as_byte_buffer();

        if (!buffer)
        {
            return 1;
        }

        stack_push_int(buffer->position());

        return 0;
    }


}


namespace sorth::run_time::abi::words
{


    void register_buffer_words(const RuntimeWordRegistrar& registrar)
    {
        registrar("buffer.new", "word_buffer_new");
        registrar("buffer.int!", "word_buffer_write_int");
        registrar("buffer.int@", "word_buffer_read_int");
        registrar("buffer.float!", "word_buffer_write_float");
        registrar("buffer.float@", "word_buffer_read_float");
        registrar("buffer.string!", "word_buffer_write_string");
        registrar("buffer.string@", "word_buffer_read_string");
        registrar("buffer.size@", "word_buffer_get_size");
        registrar("buffer.position!", "word_buffer_set_position");
        registrar("buffer.position@", "word_buffer_get_position");
    }


}


#include "sorth-runtime.h"
#include "runtime-words.h"



using namespace sorth::run_time::data_structures;



extern "C"
{


    using WordType = int8_t (*)();
    extern WordType word_table[];



    int8_t word_thread_new()
    {
        set_last_error("Word thread_new is currently not implemented.");
        return 1;
    }


    int8_t word_thread_push_to()
    {
        set_last_error("Word thread_push_to is currently not implemented.");
        return 1;
    }


    int8_t word_thread_pop_from()
    {
        set_last_error("Word thread_pop_from is currently not implemented.");
        return 1;
    }


    int8_t word_thread_push()
    {
        set_last_error("Word thread_push is currently not implemented.");
        return 1;
    }


    int8_t word_thread_pop()
    {
        set_last_error("Word thread_pop is currently not implemented.");
        return 1;
    }


    int8_t word_user_env_read()
    {
        Value key_value;

        auto pop_result = stack_pop(&key_value);

        if (pop_result)
        {
            return 1;
        }

        if (!key_value.is_string())
        {
            set_last_error("Word user_env@ expected a string value for the environment "
                           "variable key.");

            return 1;
        }

        auto key = key_value.get_string();
        const char* value = std::getenv(key.c_str());

        if (value)
        {
            stack_push_string(value);
        }
        else
        {
            Value none;
            stack_push(&none);
        }

        return 0;
    }


    int8_t word_user_os_read()
    {
        #ifdef __APPLE__

            interpreter->push(std::string("macOS"));

        #elif __linux__

            stack_push_string("Linux");

        #elif _WIN32 || _WIN64

            interpreter->push(std::string("Windows"));

        #else

            interpreter->push(std::string("Unknown"));

        #endif

        return 0;
    }


    int8_t word_user_cwd()
    {
        auto cwd = std::filesystem::current_path();

        stack_push_string(cwd.c_str());

        return 0;
    }


    int8_t word_throw()
    {
        Value message_value;

        auto pop_result = stack_pop(&message_value);

        if (pop_result)
        {
            return 1;
        }

        if (!message_value.is_string())
        {
            set_last_error("Word throw expected a string value for the exception message.");

            return 1;
        }

        set_last_error(message_value.get_string().c_str());

        return 1;
    }


    int8_t word_exit_success()
    {
        stack_push_int(static_cast<int64_t>(EXIT_SUCCESS));

        return 0;
    }


    int8_t word_exit_failure()
    {
        stack_push_int(static_cast<int64_t>(EXIT_FAILURE));

        return 0;
    }


    int8_t word_none()
    {
        Value value;

        stack_push(&value);

        return 0;
    }


    int8_t word_true()
    {
        Value value(true);

        stack_push(&value);

        return 0;
    }


    int8_t word_false()
    {
        Value value(false);

        stack_push(&value);

        return 0;
    }


    int8_t execute()
    {
        int64_t index;
        auto pop_result = stack_pop_int(&index);

        if (pop_result)
        {
            set_last_error("Word execute failed to pop the word index from the stack.");
            return 1;
        }

        auto word_ptr = word_table[index];

        if (!word_ptr)
        {
            set_last_error(("Word execute failed to find the word at index " +
                           std::to_string(index) + ".").c_str());

            return 1;
        }

        return word_table[index]();
    }


}


namespace sorth::run_time::abi::words
{


    void register_runtime_execution_words(const RuntimeWordRegistrar& registrar)
    {
        registrar("thread.new", "word_thread_new");
        registrar("thread.push-to", "word_thread_push_to");
        registrar("thread.pop-from", "word_thread_pop_from");
        registrar("thread.push", "word_thread_push");
        registrar("thread.pop", "word_thread_pop");

        registrar("user.env@", "word_user_env_read");
        registrar("user.os", "word_user_os_read");
        registrar("user.cwd", "word_user_cwd");

        registrar("throw", "word_throw");
        registrar("exit_success", "word_exit_success");
        registrar("exit_failure", "word_exit_failure");
        registrar("none", "word_none");
        registrar("true", "word_true");
        registrar("false", "word_false");
        registrar("execute", "execute");
    }


}


#include "sorth-runtime.h"



namespace
{


    thread_local std::optional<std::string> last_error;


}


extern "C"
{


    void set_last_error(const char* message)
    {
        last_error = message;
    }


    const char* get_last_error()
    {
        return last_error.value().c_str();
    }


    void push_last_error()
    {
        if (last_error.has_value())
        {
            stack_push_string(get_last_error());
            clear_last_error();
        }
    }


    bool is_error_set()
    {
        return last_error.has_value();
    }


    void clear_last_error()
    {
        last_error.reset();
    }


}


namespace sorth::run_time::abi
{


    void set_last_error(const std::string& error)
    {
        last_error = error;
    }


}

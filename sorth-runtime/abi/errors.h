
#pragma once



extern "C"
{


    void set_last_error(const char* message);


    const char* get_last_error();


    void push_last_error();


    bool is_error_set();


    void clear_last_error();


    void debug_print(const char* message);


    void debug_print_bool(bool value);


}


namespace sorth::run_time::abi
{


    void set_last_error(const std::string& error);


}

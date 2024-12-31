
#pragma once



extern "C"
{


    void stack_initialize();


    void stack_push(const sorth::run_time::data_structures::Value* value);


    void stack_push_int(int64_t value);


    void stack_push_double(double value);


    void stack_push_bool(bool value);


    void stack_push_string(const char* value);


    int8_t stack_pop(sorth::run_time::data_structures::Value* value);


    int8_t stack_pop_int(int64_t* value);


    int8_t stack_pop_bool(bool* value);

}

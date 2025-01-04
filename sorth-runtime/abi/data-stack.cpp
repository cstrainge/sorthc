
#include "sorth-runtime.h"



using namespace sorth::run_time::data_structures;


namespace
{


    thread_local std::vector<Value> data_stack;


}


extern "C"
{


    void stack_initialize()
    {
        data_stack.clear();
        data_stack.reserve(1024);
    }


    void stack_push(const Value* value)
    {
        data_stack.push_back(*value);
    }


    void stack_push_int(int64_t value)
    {
        data_stack.push_back(value);
    }


    void stack_push_double(double value)
    {
        data_stack.push_back(value);
    }


    void stack_push_bool(bool value)
    {
        data_stack.push_back(value);
    }


    void stack_push_string(const char* value)
    {
        data_stack.push_back(std::string(value));
    }


    int8_t stack_pop(Value* value)
    {
        if (data_stack.empty())
        {
            return 1;
        }

        *value = data_stack.back();
        data_stack.pop_back();

        return 0;
    }


    int8_t stack_pop_int(int64_t* value)
    {
        if (data_stack.empty())
        {
            set_last_error("Stack underflow.");
            return 1;
        }

        auto new_value = data_stack.back();
        data_stack.pop_back();

        if (!new_value.is_numeric())
        {
            set_last_error("Value is not an integer.");
            return 1;
        }

        *value = new_value.get_int();

        return 0;
    }


    int8_t stack_pop_bool(bool* value)
    {
        if (data_stack.empty())
        {
            return 1;
        }

        auto new_value = data_stack.back();
        data_stack.pop_back();

        if (!new_value.is_numeric())
        {
            return 1;
        }

        *value = new_value.get_bool();

        return 0;
    }


    int8_t stack_pop_double(double* value)
    {
        if (data_stack.empty())
        {
            return 1;
        }

        auto new_value = data_stack.back();
        data_stack.pop_back();

        if (!new_value.is_numeric())
        {
            return 1;
        }

        *value = new_value.get_double();

        return 0;
    }


}

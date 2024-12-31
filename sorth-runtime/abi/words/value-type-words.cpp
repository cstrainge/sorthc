
#include "sorth-runtime.h"
#include "value-type-words.h"



using namespace sorth::run_time::data_structures;


extern "C"
{


       uint8_t word_value_is_number()
        {
            Value value;

            auto pop_result = stack_pop(&value);

            if (pop_result)
            {
                return 1;
            }

            stack_push_bool(value.is_numeric());

            return 0;
        }


        uint8_t word_value_is_boolean()
        {
            Value value;

            auto pop_result = stack_pop(&value);

            if (pop_result)
            {
                return 1;
            }

            stack_push_bool(value.is_bool());

            return 0;
        }


        uint8_t word_value_is_string()
        {
            Value value;

            auto pop_result = stack_pop(&value);

            if (pop_result)
            {
                return 1;
            }

            stack_push_bool(value.is_string());

            return 0;
        }


//        uint8_t word_value_is_thread_id()
//        {
//            Value value;
//
//            auto pop_result = stack_pop(&value);
//
//            if (pop_result)
//            {
//                return 1;
//            }
//
//            stack_push_bool(value.is_thread_id());
//
//            return 0;
//        }


        uint8_t word_value_is_structure()
        {
            Value value;

            auto pop_result = stack_pop(&value);

            if (pop_result)
            {
                return 1;
            }

            stack_push_bool(value.is_structure());

            return 0;
        }


        uint8_t word_value_is_array()
        {
            Value value;

            auto pop_result = stack_pop(&value);

            if (pop_result)
            {
                return 1;
            }

            stack_push_bool(value.is_array());

            return 0;
        }


        uint8_t word_value_is_buffer()
        {
            Value value;

            auto pop_result = stack_pop(&value);

            if (pop_result)
            {
                return 1;
            }

            stack_push_bool(value.is_byte_buffer());

            return 0;
        }


        uint8_t word_value_is_hash_table()
        {
            Value value;

            auto pop_result = stack_pop(&value);

            if (pop_result)
            {
                return 1;
            }

            stack_push_bool(value.is_hash_table());

            return 0;
        }


        uint8_t word_value_copy()
        {
            Value original;

            auto pop_result = stack_pop(&original);

            if (pop_result)
            {
                return 1;
            }

            Value copy = original.deep_copy();

            stack_push(&copy);

            return 0;
        }


}


namespace sorth::run_time::abi::words
{


    void register_value_type_words(const RuntimeWordRegistrar& registrar)
    {
        registrar("value.is-number?", "word_value_is_number");
        registrar("value.is-boolean?", "word_value_is_boolean");
        registrar("value.is-string?", "word_value_is_string");
        registrar("value.is-structure?", "word_value_is_structure");
        registrar("value.is-array?", "word_value_is_array");
        registrar("value.is-buffer?", "word_value_is_buffer");
        registrar("value.is-hash-table?", "word_value_is_hash_table");
        registrar("value.copy", "word_value_copy");
    }


}

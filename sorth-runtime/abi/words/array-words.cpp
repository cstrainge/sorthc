
#include "sorth-runtime.h"
#include "array-words.h"



using namespace sorth::run_time::data_structures;



namespace
{


    uint8_t stack_pop_as_size(size_t* value)
    {
        int64_t int_value;

        auto pop_result = stack_pop_int(&int_value);

        if (pop_result)
        {
            return 1;
        }

        *value = static_cast<size_t>(int_value);

        return 0;
    }


    ArrayPtr stack_pop_as_array()
    {
        Value value;

        auto pop_result = stack_pop(&value);

        if (pop_result)
        {
            return nullptr;
        }

        if (!value.is_array())
        {
            set_last_error("Expected an array value.");
            return nullptr;
        }

        return value.get_array();
    }


    int8_t check_bounds(size_t index, const ArrayPtr& array)
    {
        if (index >= array->size())
        {
            std::stringstream stream;

            stream << "Index out of bounds for array value.";

            set_last_error(stream.str().c_str());

            return 1;
        }

        return 0;
    }


}


extern "C"
{


        uint8_t word_array_new()
        {
            size_t count;
            auto pop_result = stack_pop_as_size(&count);

            if (pop_result)
            {
                return 1;
            }

            Value array_ptr = std::make_shared<Array>(count);

            stack_push(&array_ptr);

            return 0;
        }


        uint8_t word_array_size()
        {
            auto array = stack_pop_as_array();

            if (!array)
            {
                return 1;
            }

            stack_push_int(array->size());

            return 0;
        }


        uint8_t word_array_write_index()
        {
            auto array = stack_pop_as_array();
            size_t index;
            Value new_value;

            auto pop_result_1 = stack_pop_as_size(&index);
            auto pop_result_2 = stack_pop(&new_value);

            if (pop_result_1 || pop_result_2 || !array || check_bounds(index, array))
            {
                return 1;
            }

            (*array)[index] = new_value;

            return 0;
        }


        uint8_t word_array_read_index()
        {
            auto array = stack_pop_as_array();
            size_t index;

            auto pop_result = stack_pop_as_size(&index);

            if (pop_result || !array || check_bounds(index, array))
            {
                return 1;
            }

            stack_push(&(*array)[index]);

            return 0;
        }


        uint8_t word_array_insert()
        {
            auto array = stack_pop_as_array();
            size_t index;
            Value value;

            auto pop_result_1 = stack_pop_as_size(&index);
            auto pop_result_2 = stack_pop(&value);

            if (pop_result_1 || pop_result_2 || !array)
            {
                return 1;
            }

            array->insert(index, value);

            return 0;
        }


        uint8_t word_array_delete()
        {
            auto array = stack_pop_as_array();
            size_t index;
            auto pop_result = stack_pop_as_size(&index);

            if (pop_result || !array || check_bounds(index, array))
            {
                return 1;
            }

            array->remove(index);

            return 0;
        }


        uint8_t word_array_resize()
        {
            auto array = stack_pop_as_array();
            size_t new_size;

            auto pop_result = stack_pop_as_size(&new_size);

            if (pop_result || !array)
            {
                return 1;
            }

            array->resize(new_size);

            return 0;
        }


        uint8_t word_array_plus()
        {
            auto array_src = stack_pop_as_array();
            auto array_dest = stack_pop_as_array();

            if (!array_src || !array_dest)
            {
                return 1;
            }

            auto orig_size = array_dest->size();
            auto new_size = orig_size + array_src->size();

            array_dest->resize(new_size);

            for (auto i = orig_size; i < new_size; ++i)
            {
                (*array_dest)[i] = (*array_src)[i - orig_size].deep_copy();
            }

            Value value = array_dest;

            stack_push(&value);

            return 0;
        }


        uint8_t word_array_compare()
        {
            auto array_a = stack_pop_as_array();
            auto array_b = stack_pop_as_array();

            if (!array_a || !array_b)
            {
                return 1;
            }

            stack_push_bool(array_a == array_b);

            return 0;
        }


        uint8_t word_push_front()
        {
            auto array = stack_pop_as_array();
            Value value;

            auto pop_result = stack_pop(&value);

            if (pop_result || !array)
            {
                return 1;
            }

            array->push_front(value);

            return 0;
        }


        uint8_t word_push_back()
        {
            auto array = stack_pop_as_array();
            Value value;

            auto pop_result = stack_pop(&value);

            if (pop_result || !array)
            {
                return 1;
            }

            array->push_back(value);

            return 0;
        }


        uint8_t word_pop_front()
        {
            auto array = stack_pop_as_array();

            if (!array)
            {
                return 1;
            }

            if (array->size() == 0)
            {
                set_last_error("Pop from empty array.");
                return 1;
            }

            Value value = array->pop_front();

            stack_push(&value);

            return 0;
        }


        uint8_t word_pop_back()
        {
            auto array = stack_pop_as_array();

            if (!array)
            {
                return 1;
            }

            if (array->size() == 0)
            {
                set_last_error("Pop from empty array.");
                return 1;
            }

            Value value = array->pop_back();

            stack_push(&value);

            return 0;
        }


}


namespace sorth::run_time::abi::words
{


    void register_array_words(const RuntimeWordRegistrar& registrar)
    {
        registrar("[].new", "word_array_new");
        registrar("[].size@", "word_array_size");
        registrar("[]!", "word_array_write_index");
        registrar("[]@", "word_array_read_index");
        registrar("[].insert", "word_array_insert");
        registrar("[].delete", "word_array_delete");
        registrar("[].size!", "word_array_resize");
        registrar("[].+", "word_array_plus");
        registrar("[].=", "word_array_compare");
        registrar("[].push_front!", "word_push_front");
        registrar("[].push_back!", "word_push_back");
        registrar("[].pop_front!", "word_pop_front");
        registrar("[].pop_back!", "word_pop_back");
    }


}

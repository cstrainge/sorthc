
#include "sorth-runtime.h"
#include "structure-words.h"



using namespace sorth::run_time::data_structures;



namespace
{


    StructurePtr stack_pop_as_structure()
    {
        Value value;

        auto pop_result = stack_pop(&value);

        if (pop_result)
        {
            return nullptr;
        }

        if (!value.is_structure())
        {
            set_last_error("Expected a structure value.");
            return nullptr;
        }

        return value.get_structure();
    }


}


extern "C"
{


        using WordType = int8_t (*)();
        extern WordType word_table[];


        uint8_t word_read_field()
        {
            auto object = stack_pop_as_structure();
            int64_t field_index;

            auto pop_result = stack_pop_int(&field_index);

            if (!object || pop_result)
            {
                return 1;
            }

            if (field_index < 0 || field_index >= object->fields.size())
            {
                set_last_error("Structure field index out of range.");
                return 1;
            }

            stack_push(&object->fields[field_index]);

            return 0;
        }


        uint8_t word_write_field()
        {
            auto object = stack_pop_as_structure();
            int64_t field_index;

            auto pop_result1 = stack_pop_int(&field_index);

            if (!object || pop_result1)
            {
                return 1;
            }

            if (field_index < 0 || field_index >= object->fields.size())
            {
                set_last_error("Structure field index out of range.");
                return 1;
            }

            auto pop_result2 = stack_pop(&object->fields[field_index]);

            return pop_result2;
        }


        uint8_t word_structure_iterate()
        {
            auto object = stack_pop_as_structure();
            int64_t word_index;

            auto pop_result = stack_pop_int(&word_index);

            if (!object || pop_result)
            {
                return 1;
            }

            auto handler = word_table[word_index];

            auto& data_type = object->definition;

            for (size_t i = 0; i < data_type->fieldNames.size(); ++i)
            {
                stack_push_string(data_type->fieldNames[i].c_str());
                stack_push(&object->fields[i]);

                handler();
            }

            return 0;
        }


        uint8_t word_structure_field_exists()
        {
            auto object = stack_pop_as_structure();
            Value value;

            auto pop_result = stack_pop(&value);

            if (!object || pop_result)
            {
                return 1;
            }

            auto field_name = value.get_string();

            bool found = false;

            for (const auto& name : object->definition->fieldNames)
            {
                if (name == field_name)
                {
                    found = true;
                    break;
                }
            }

            stack_push_bool(found);

            return 0;
        }


        uint8_t word_structure_compare()
        {
            auto a = stack_pop_as_structure();
            auto b = stack_pop_as_structure();

            if (!a || !b)
            {
                return 1;
            }

            stack_push_bool(a == b);

            return 0;
        }


}


namespace sorth::run_time::abi::words
{


    void register_structure_words(const RuntimeWordRegistrar& registrar)
    {
        registrar("#@", "word_read_field");
        registrar("#!", "word_write_field");
        registrar("#.iterate", "word_structure_iterate");
        registrar("#.field-exists?", "word_structure_field_exists");
        registrar("#.=", "word_structure_compare");
    }


}

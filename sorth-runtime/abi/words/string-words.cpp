
#include "sorth-runtime.h"
#include "string-words.h"



using namespace sorth::run_time::data_structures;


namespace
{


    uint8_t stack_pop_string(std::string& string)
    {
        Value value;

        auto pop_result = stack_pop(&value);

        if (pop_result)
        {
            return 1;
        }

        if (!value.is_string())
        {
            set_last_error("Expected a string value.");

            return 1;
        }

        string = value.get_string();

        return 0;
    }


}


extern "C"
{


        uint8_t word_string_length()
        {
            std::string string;

            if (stack_pop_string(string))
            {
                return 1;
            }

            stack_push_int(static_cast<int64_t>(string.size()));

            return 0;
        }


        uint8_t word_string_insert()
        {
            std::string string;
            int64_t position;
            std::string sub_string;

            auto pop_result_1 = stack_pop_string(string);
            auto pop_result_2 = stack_pop_int(&position);
            auto pop_result_3 = stack_pop_string(sub_string);

            if (pop_result_1 || pop_result_2 || pop_result_3)
            {
                return 1;
            }

            string.insert(position, sub_string);

            stack_push_string(string.c_str());

            return 0;
        }


        uint8_t word_string_remove()
        {
            std::string string;
            int64_t start;
            int64_t count;

            auto pop_result_1 = stack_pop_string(string);
            auto pop_result_2 = stack_pop_int(&start);
            auto pop_result_3 = stack_pop_int(&count);

            if (pop_result_1 || pop_result_2 || pop_result_3)
            {
                return 1;
            }

            if (   (start < 0)
                || (start > string.size()))
            {
                std::stringstream message;

                message << "string.remove start index, " << start << ", outside of the string.";
                set_last_error(message.str().c_str());

                return 1;
            }

            if (   (count != std::string::npos)
                && ((start + count) > string.size()))
            {
                std::stringstream message;

                message << "string.remove end index, " << start + count << ", outside of the string.";
                set_last_error(message.str().c_str());

                return 1;
            }

            string.erase(start, count);

            stack_push_string(string.c_str());

            return 0;
        }


        uint8_t word_string_find()
        {
            std::string string;
            std::string search_str;

            auto pop_result_1 = stack_pop_string(string);
            auto pop_result_2 = stack_pop_string(search_str);

            if (pop_result_1 || pop_result_2)
            {
                return 1;
            }

            stack_push_int(static_cast<int64_t>(string.find(search_str, 0)));

            return 0;
        }


        uint8_t word_string_index_read()
        {
            std::string string;
            int64_t position;

            auto pop_result_1 = stack_pop_string(string);
            auto pop_result_2 = stack_pop_int(&position);

            if (pop_result_1 || pop_result_2)
            {
                return 1;
            }

            if ((position < 0) || (position >= string.size()))
            {
                set_last_error("String index out of range.");

                return 1;
            }

            std::string new_str(1, string[position]);
            stack_push_string(new_str.c_str());

            return 0;
        }


        uint8_t word_string_add()
        {
            std::string str_b;
            std::string str_a;

            auto pop_result_1 = stack_pop_string(str_b);
            auto pop_result_2 = stack_pop_string(str_a);

            if (pop_result_1 || pop_result_2)
            {
                return 1;
            }

            stack_push_string((str_a + str_b).c_str());

            return 0;
        }


        uint8_t word_string_to_number()
        {
            std::string string;

            if (stack_pop_string(string))
            {
                return 1;
            }

            if (string.find('.', 0) != std::string::npos)
            {
                stack_push_double(std::atof(string.c_str()));
            }
            else
            {
                stack_push_int(static_cast<int64_t>(std::strtoll(string.c_str(), nullptr, 10)));
            }

            return 0;
        }


        uint8_t word_to_string()
        {
            Value value;
            std::stringstream stream;

            auto pop_result = stack_pop(&value);

            if (pop_result)
            {
                return 1;
            }

            stream << value;
            stack_push_string(stream.str().c_str());

            return 0;
        }


        int8_t word_string_npos()
        {
            stack_push_int(std::string::npos);

            return 0;
        }


        uint8_t word_hex()
        {
            Value value;

            auto pop_result = stack_pop(&value);

            if (pop_result)
            {
                return 1;
            }

            std::stringstream stream;

            if (value.is_string())
            {
                auto string_value = value.get_string();

                for (auto next : string_value)
                {
                    stream << std::hex << (int)next << std::dec;
                }
            }
            else if (value.is_numeric())
            {
                auto int_value = value.get_int();
                stream << std::hex << int_value << std::dec << " ";
            }
            else
            {
                set_last_error("Expected a string or numeric value.");

                return 1;
            }

            stack_push_string(stream.str().c_str());

            return 0;
        }


}


namespace sorth::run_time::abi::words
{


    void register_string_words(const RuntimeWordRegistrar& registrar)
    {
        registrar("string.size@", "word_string_length");
        registrar("string.[]!", "word_string_insert");
        registrar("string.remove", "word_string_remove");
        registrar("string.find", "word_string_find");
        registrar("string.[]@", "word_string_index_read");
        registrar("string.+", "word_string_add");
        registrar("string.to_number", "word_string_to_number");
        registrar("value.to-string", "word_to_string");
        registrar("string.npos", "word_string_npos");
        registrar("hex", "word_hex");
    }


}

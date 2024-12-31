
#include "sorth-runtime.h"
#include "runtime-words.h"



extern "C"
{


    using WordType = int8_t (*)();


    extern WordType word_table[];



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
        registrar("execute", "execute");
    }


}

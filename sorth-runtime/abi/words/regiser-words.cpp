
#include "sorth-runtime.h"
#include "array-words.h"
#include "byte-buffer-words.h"
#include "hash-table-words.h"
#include "math-logic-words.h"
#include "runtime-words.h"
#include "stack-words.h"
#include "string-words.h"
#include "structure-words.h"
#include "terminal-words.h"
#include "value-type-words.h"
#include "posix-words.h"



namespace sorth::run_time::abi::words
{


    namespace
    {


        // Store the command line arguments in an array variable so that they can be accessed by
        // the compiled Forth program.
        data_structures::Value argument_array;


        // Push the gathered command line arguments onto the data stack.
        uint8_t word_get_arguments()
        {
            stack_push(&argument_array);

            return 0;
        }


    }



    // Called by the sorthc compiler to pre-register all of the native words that are available to
    // the user's Forth code from the run-time..
    void register_runtime_words(const RuntimeWordRegistrar& registrar)
    {
        register_array_words(registrar);
        register_buffer_words(registrar);
        register_hash_table_words(registrar);
        register_math_logic_words(registrar);
        register_runtime_execution_words(registrar);
        register_stack_words(registrar);
        register_string_words(registrar);
        register_structure_words(registrar);
        register_terminal_words(registrar);
        register_value_type_words(registrar);
        register_posix_words(registrar);

        registrar("sorth.args", "word_get_arguments");
    }



    // Called at run-time startup to make the command line arguments available to the Forth program.
    void register_command_line_arguments(int argc, char* argv[])
    {
        auto new_array = std::make_shared<data_structures::Array>(argc);

        for (int i = 0; i < argc; ++i)
        {
            (*new_array)[i] = argv[i];
        }

        argument_array = new_array;
    }


}

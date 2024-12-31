
#include "sorth-runtime.h"
#include "array-words.h"
#include "byte-buffer-words.h"
#include "hash-table-words.h"
#include "math-logic-words.h"
#include "runtime-words.h"
#include "stack-words.h"
#include "string-words.h"
#include "structure-words.h"
#include "value-type-words.h"



namespace sorth::run_time::abi::words
{



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
        register_value_type_words(registrar);
    }



    // Called at run-time to make the command line arguments available to the Forth program.
    void register_command_line_arguments(int argc, char* argv[])
    {
        //
    }


}

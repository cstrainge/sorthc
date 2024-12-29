
#include "sorth-runtime.h"



namespace sorth::run_time::abi::words
{


    // Called by the sorthc compiler to pre-register all of the native words that are available to
    // the user's Forth code from the run-time..
    void register_runtime_words(const RuntimeWordRegistrar& registrar)
    {
        //
    }



    // Called at run-time to make the command line arguments available to the Forth program.
    void register_command_line_arguments(int argc, char* argv[])
    {
        //
    }


}

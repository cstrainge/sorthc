
#pragma once



namespace sorth::compilation::run_time
{


    class CompilerRuntime;


    namespace built_in_words
    {


        // Register the built-in words with the compiler run-time.
        void register_words(CompilerRuntime& runtime);


    }


}

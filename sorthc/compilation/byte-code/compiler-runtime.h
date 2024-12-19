
#pragma once



namespace sorthc::compilation::byte_code
{


    // Run-time internal to the compiler responsible for managing the execution of immediate words
    // found within the source code.
    //
    // This class handles typical Forth state, including the data stack, run-time variables, and
    // a dictionary of immediate words, and words that are useful for the immediate words.  This
    // won't be the full Strange Forth run-time, but it will be enough to run the immediate words.
    class CompilerRuntime
    {
        //
    };


}

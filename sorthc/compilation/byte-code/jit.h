
#pragma once



namespace sorthc::compilation::byte_code
{


    // The JIT compiler for Strange Forth immediate words, converting them from byte-cde into native
    // code for execution by the compiler's runtime.
    //
    // Non-immediate words take a different path through the compiler and are not JIT compiled.
    // Instead they are compiled as a whole with their module, allowing for more aggressive
    // optimization.
    class Jit
    {
        //
    };


}

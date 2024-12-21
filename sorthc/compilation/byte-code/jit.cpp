
#include "sorthc.h"



namespace sorthc::compilation::byte_code
{


    Jit& get_jit_engine()
    {
        static Jit jit;

        return jit;
    }


    Jit::Jit()
    {
    }


    void Jit::jit_compile(run_time::CompilerRuntime& runtime, const Construction& construction)
    {
    }


    run_time::WordHandler Jit::jit_compile(run_time::CompilerRuntime& runtime, const Script& script)
    {
        auto null_handler = [](run_time::CompilerRuntime&) -> void
        {
            // Do nothing.
        };

        return null_handler;
    }


}

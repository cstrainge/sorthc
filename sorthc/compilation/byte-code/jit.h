
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
        private:
            // llvm stuff...

        public:
            Jit();
            Jit(const Jit& jit) = delete;
            Jit(Jit&& jit) = delete;
            ~Jit() = default;

        public:
            Jit& operator =(const Jit& jit) = delete;
            Jit& operator =(Jit&& jit) = delete;

        public:
            // JIT compile a word construction into native code and register it for running within
            // the compiler's runtime.
            void jit_compile(run_time::CompilerRuntime& runtime, const Construction& construction);

            // JIT compile a script into native code for running within the compiler's runtime.
            // Register all of the scripts non-immediate words for execution for other immediate
            // words.
            run_time::WordHandler jit_compile(run_time::CompilerRuntime& runtime,
                                              const Script& script);
    };


    Jit& get_jit_engine();


}


#pragma once



namespace sorth::compilation
{


    // The Strange Forth compiler, which compiles Strange Forth source code an executable runnable
    // by the operating system.
    //
    // Compilation is a three-pass process.  The first pass tokenizes the source code and generates
    // a sequence of Strange Forth VM byte-code instructions.  The second pass converts these
    // instructions into LLVM intermediate representation for further optimization.  The third pass
    // takes the optimized LLVM IR and compiles it into native code and links it with the Strange
    // Forth run-time.  Writing the result to an executable file.
    //
    // During the first pass immediate words are jit-compiled against the compiler's internal
    // run-time and are used to help parse the source code and generate additional byte-code
    // instructions.  The compiler run-time is limited vs the full Strange Forth run-time, but it is
    // enough to run the immediate words.
    class Compiler
    {
        private:
            // Runtime for the compiler, which is used to manage the execution of immediate words
            // during the byte-code compilation phase.
            run_time::CompilerRuntime runtime;

        public:
            // Create a new compiler with the given system path standard library directory.
            Compiler(const std::filesystem::path& system_path) noexcept;
            Compiler(Compiler& compiler) = delete;
            Compiler(Compiler&& compiler) = delete;
            ~Compiler() noexcept = default;

        public:
            Compiler& operator =(Compiler& compiler) = delete;
            Compiler& operator =(Compiler&& compiler) = delete;

        public:
            // Append a search path to the compiler's search paths.
            void append_search_path(const std::filesystem::path& path);

        public:
            // Compile a Strange Forth source file and any files it includes into an executable
            // runnable by the operating system.
            void compile(const std::filesystem::path& source_path,
                         const std::filesystem::path& output_path);

        private:
            // Find a file in the search paths.
            void find_file(const std::filesystem::path& path);
    };


}

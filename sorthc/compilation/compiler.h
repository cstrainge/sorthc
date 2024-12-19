
#pragma once



namespace sorthc::compilation
{


    class Compiler
    {
        private:
            // Search paths for the compiler to find source files and included files.
            run_time::SearchPaths search_paths;

            // Runtime for the compiler, which is used to manage the execution of immediate words
            // during the byte-code compilation phase.
            run_time::CompilerRuntime runtime;

        public:
            Compiler(const std::filesystem::path& system_path) noexcept;
            Compiler(Compiler& compiler) = delete;
            Compiler(Compiler&& compiler) = delete;
            ~Compiler() noexcept = default;

        public:
            Compiler& operator=(Compiler& compiler) = delete;
            Compiler& operator=(Compiler&& compiler) = delete;

        public:
            void append_search_path(const std::filesystem::path& path);

        public:
            // Compile a Strange Forth source file and any files it includes into an executable
            // runnable by the operating system.
            void compile(const std::filesystem::path& source_path,
                         const std::filesystem::path& output_path);

        private:
            void find_file(const std::filesystem::path& path);
    };


}

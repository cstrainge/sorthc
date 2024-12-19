
#pragma once


namespace sorthc::compilation::byte_code
{


    class Script;


}


namespace sorthc::compilation::run_time
{


    using SearchPaths = std::vector<std::filesystem::path>;


    using ScriptCache = std::unordered_map<std::string, byte_code::Script>;


    // Run-time internal to the compiler responsible for managing the execution of immediate words
    // found within the source code.
    //
    // This class handles typical Forth state, including the data stack, run-time variables, and
    // a dictionary of immediate words, and words that are useful for the immediate words.  This
    // won't be the full Strange Forth run-time, but it will be enough to run the immediate words.
    class CompilerRuntime
    {
        private:
            source::Location location;

        public:
            CompilerRuntime() noexcept;
            CompilerRuntime(CompilerRuntime& runtime) = delete;
            CompilerRuntime(CompilerRuntime&& runtime) = delete;
            ~CompilerRuntime() noexcept = default;

        public:
            CompilerRuntime& operator=(CompilerRuntime& runtime) = delete;
            CompilerRuntime& operator=(CompilerRuntime&& runtime) = delete;

        public:
            const source::Location& get_location() const noexcept;

        public:
            ScriptCache compile_script(const SearchPaths& paths, const std::filesystem::path& path);
    };


}

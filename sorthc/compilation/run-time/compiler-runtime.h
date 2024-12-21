
#pragma once



namespace sorthc::compilation::run_time
{


    // Definition of the search path list, used in finding files during compilation.
    using SearchPaths = std::vector<std::filesystem::path>;


    // Define a cache of scripts that have been compiled into byte-code.
    using ScriptCache = std::unordered_map<std::filesystem::path, byte_code::Script>;


    // Reference to a function that actually handles the execution for a word.
    using WordHandler = std::function<void(CompilerRuntime&)>;


    // Information about a word handler that is stored in the handler list.
    struct WordHandlerInfo
    {
        // Location in the source code where the word was defined.
        source::Location location;

        // Name of the word.
        std::string name;


        // The function to call to handle the word.
        WordHandler handler;
    };


    // Definition of the word handler list used by the runtime to store handlers referenced by the
    // word dictionary.
    using HandlerList = ContextualList<WordHandlerInfo>;


    // Used by the runtime for debugging purposes, this structure holds information about the
    // current items within the call stack of Forth words being executed.
    struct CallStackItem
    {
        // Location in the source code the word was defined.
        source::Location location;

        // The name of the word being executed.
        std::string name;
    };


    // Stack of call stack items, used to track the execution of Forth words by the compiler
    // run-time.
    using CallStack = std::stack<CallStackItem>;


    // Definition of the variable list used by the runtime to store variables that are defined
    // within it.  These variables are contextual, meaning that they are only valid within the
    // context they are defined in.
    using VariableList = ContextualList<Value>;


    // Definition of the value stack used by the runtime to store values that are pushed and popped
    // from the Forth data stack.
    using ValueStack = std::vector<Value>;


    // Run-time internal to the compiler responsible for managing the execution of immediate words
    // found within the source code.
    //
    // This class handles typical Forth state, including the data stack, run-time variables, and
    // a dictionary of immediate words, and words that are useful for the immediate words.  This
    // won't be the full Strange Forth run-time, but it will be enough to run the immediate words.
    class CompilerRuntime
    {
        private:
            // Search paths for the compiler to find source files and included files.
            SearchPaths search_paths;

            // Cache of byte-code compiled scripts.
            ScriptCache script_cache;

            // Dictionary of words known to the compiler run-time.
            Dictionary dictionary;

            // List of handlers for words defined in the dictionary.
            HandlerList handlers;

            // List of variables defined within the runtime.
            VariableList variables;

            // Stack of values that are pushed and popped from the data stack by Forth words runing
            // in the compiler's run-time.
            ValueStack data_stack;

            // Stack of call stack items, used to track the execution of Forth words by the compiler
            // run-time.
            CallStack call_stack;

            // Location of the compiler/run-time in the source code.
            source::Location location;

            // Stack of contexts that are being compiled.
            byte_code::ContextStack compile_contexts;

        public:
            // Create the compiler run-time and load the standard library from the given system
            // path.
            CompilerRuntime(const std::filesystem::path& system_path);
            CompilerRuntime(CompilerRuntime& runtime) = delete;
            CompilerRuntime(CompilerRuntime&& runtime) = delete;
            ~CompilerRuntime() noexcept = default;

        public:
            CompilerRuntime& operator =(CompilerRuntime& runtime) = delete;
            CompilerRuntime& operator =(CompilerRuntime&& runtime) = delete;

        public:
            // Get the current source code location the compiler is at.
            const source::Location& get_location() const noexcept;

            // Get the cache of compiled scripts.
            const ScriptCache& get_script_cache() const noexcept;

        public:
            // Append a search path to the run-time's search path collection.
            void append_search_path(const std::filesystem::path& path);

        public:
            // Find and byte-code compile a script file.  Once compiled the script is added to the
            // cache.  If the script is already in the cache, nothing is done.
            void compile_script(const std::filesystem::path& path);

        public:
            // Add a word to the run-time dictionary.
            void add_word(const WordHandlerInfo& info,
                          WordExecutionContext execution_context,
                          WordVisibility visibility,
                          WordType type,
                          WordContextManagement context_management);

            // Add a word to the run-time dictionary.
            void add_word(const std::string& name,
                          const source::Location& location,
                          WordHandler handler,
                          WordExecutionContext execution_context,
                          WordVisibility visibility,
                          WordType type,
                          WordContextManagement context_management);

            // Add a word to the run-time dictionary.
            void add_word(const std::string& name,
                          const std::filesystem::path& path,
                          size_t line,
                          size_t column,
                          WordHandler handler,
                          WordExecutionContext execution_context,
                          WordVisibility visibility,
                          WordType type,
                          WordContextManagement context_management);

        public:
            const std::tuple<bool, Word> find(const std::string& word_name) const noexcept;
            const std::tuple<bool, WordHandlerInfo> find(size_t handler_index) const noexcept;

        public:
            void define_variable(const std::string& name) noexcept;
            void define_constant(const std::string& name, const Value& value) noexcept;

            Value read_variable(size_t index);
            void write_variable(size_t index, Value value);

        public:
            void push(const Value& value) noexcept;

            Value pop();
            int64_t pop_as_integer();
            size_t pop_as_size();
            double pop_as_float();
            bool pop_as_bool();
            std::string pop_as_string();

        public:
            void execute(const std::string& word);
            void execute(const Word& word);
            void execute(size_t word_index);
            void execute(const WordHandlerInfo& info);

        public:
            void call_stack_push(const WordHandlerInfo& info) noexcept;
            void call_stack_push(const std::string& name,
                                 const source::Location& location) noexcept;

            void call_stack_pop();

        private:
            // Initialize the dictionary with the native words known to the compiler run-time.
            void init_dictionary();

            // Find a file in the search paths.
            std::filesystem::path find_file(const std::filesystem::path& path);
    };


    // Called by C++ code to add a new word to the compiler's runtime for access to immediate words
    // as executed by the compiler.
    #define ADD_NATIVE_WORD(runtime, name, handler) \
        runtime->add_word(name, \
                         std::filesystem::path(__FILE__), \
                         __LINE__, \
                         1, \
                         handler, \
                         sorthc::compilation::WordExecutionContext::run_time, \
                         sorthc::compilation::WordVisibility::visible, \
                         sorthc::compilation::WordType::internal, \
                         sorthc::compilation::WordContextManagement::unmanaged)


    // Called by C++ code to add a new immediate word to the compiler's runtime for access to other
    // immediate words as executed by the compiler.
    #define ADD_NATIVE_IMMEDIATE_WORD(runtime, name, handler) \
        runtime.add_word(name, \
                         __FILE__, \
                         __LINE__, \
                         1, \
                         handler, \
                         sorthc::compilation::WordExecutionContext::immediate, \
                         sorthc::compilation::WordVisibility::visible, \
                         sorthc::compilation::WordType::internal, \
                         sorthc::compilation::WordContextManagement::unmanaged)


    // Helper class to manage the call stack of the compiler run-time and ensure that the stack is
    // properly popped even in the presence of exceptions.
    class CallStackManager
    {
        private:
            CompilerRuntime& runtime;

        public:
            CallStackManager(CompilerRuntime& runtime, const WordHandlerInfo& info)
            : runtime(runtime)
            {
                runtime.call_stack_push(info);
            }

            CallStackManager(CompilerRuntime& runtime,
                             const std::string& name,
                             const source::Location& location)
            : runtime(runtime)
            {
                runtime.call_stack_push(name, location);
            }

            CallStackManager(const CallStackManager& manager) = delete;
            CallStackManager(CallStackManager&& manager) = delete;

            ~CallStackManager()
            {
                runtime.call_stack_pop();
            }

        public:
            CallStackManager& operator =(const CallStackManager& manager) = delete;
            CallStackManager& operator =(CallStackManager&& manager) = delete;
    };


}

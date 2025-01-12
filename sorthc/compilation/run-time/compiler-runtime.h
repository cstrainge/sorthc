
#pragma once



namespace sorth::compilation::run_time
{


    // Definition of the search path list, used in finding files during compilation.
    using SearchPaths = std::vector<std::filesystem::path>;


    // Define a cache of scripts that have been compiled into byte-code.
    using ScriptCache = std::unordered_map<std::filesystem::path, byte_code::ScriptPtr>;


    // Reference to a function that actually handles the execution for a word.
    using WordHandler = std::function<void(CompilerRuntime&)>;


    // Information about a word handler that is stored in the handler list.
    struct WordHandlerInfo
    {
        // Location in the source code where the word was defined.
        source::Location location;

        // Name of the word.
        std::string name;

        // The byte-code for the word, if it hasn't been jit compiled yet.
        std::optional<byte_code::ByteCode> code;


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
    using CallStack = std::list<CallStackItem>;


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
            // The script that represents the standard library.
            byte_code::ScriptPtr standard_library;

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

            bool is_building_runtime;

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
            bool get_is_building_runtime() const noexcept;

            // Get a reference to the standard library script.
            const byte_code::ScriptPtr& get_standard_library() const noexcept;

            // Get the current source code location the compiler is at.
            const source::Location& get_location() const noexcept;
            void set_location(const source::Location& value) noexcept;

            const CallStack& get_call_stack() const noexcept;

            // Get the cache of compiled scripts.
            const ScriptCache& get_script_cache() const noexcept;

        public:
            void mark_context();
            void release_context();

        public:
            // Append a search path to the run-time's search path collection.
            void append_search_path(const std::filesystem::path& path);

            // Drop the last search path from the run-time's search path collection.
            void drop_search_path();

        public:
            // Find and byte-code compile a script file.  Once compiled the script is added to the
            // cache.  If the script is already in the cache, nothing is done.
            byte_code::ScriptPtr compile_script(const std::filesystem::path& path);

        public:
            // Construct a new compilation context for the given token list representing a script.
            void create_compile_context(source::TokenList&& tokens);

            // Drop the current compile context after finishing the compilation of a script file.
            void drop_compile_context();


            // Get the current compile context.
            byte_code::Context& get_compile_context();

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
                          WordContextManagement context_management,
                          std::optional<byte_code::ByteCode>& code);

            // Add a word to the run-time dictionary.
            void add_word(const std::string& name,
                          const std::filesystem::path& path,
                          size_t line,
                          size_t column,
                          WordHandler handler,
                          WordExecutionContext execution_context,
                          WordVisibility visibility,
                          WordType type,
                          WordContextManagement context_management,
                          std::optional<byte_code::ByteCode>& code);

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
            // Find a word in the dictionary.
            const std::tuple<bool, Word> find(const std::string& word_name) const noexcept;

            // Find a word handler in the handler list.
            const std::tuple<bool, WordHandlerInfo> find(size_t handler_index) const noexcept;

        public:
            // Define a new variable in the run-time.
            void define_variable(const std::string& name) noexcept;

            // Define a new constant in the run-time.
            void define_constant(const std::string& name, const Value& value) noexcept;


            // Read from a variable from the run-time.
            Value read_variable(size_t index);

            // Write to a variable in the run-time.
            void write_variable(size_t index, Value value);

        public:
            // Push a value onto the data stack.
            void push(const Value& value) noexcept;


            // Pop a value from the data stack.
            Value pop();

            // Pop a value from the data stack and convert it to an integer.
            int64_t pop_as_integer();

            // Pop a value from the data stack and convert it to a size_t.
            size_t pop_as_size();

            // Pop a value from the data stack and convert it to a double.
            double pop_as_float();

            // Pop a value from the data stack and convert it to a boolean.
            bool pop_as_bool();

            // Pop a value from the data stack and convert it to a string.
            std::string pop_as_string();

            // Pop a value from the data stack and convert it to an array.
            ArrayPtr pop_as_array();

            // Pop a value from the data stack and convert it to a byte-code block.
            byte_code::ByteCode pop_as_byte_code();

        public:
            // Try to execute a word by name.
            void execute(const std::string& word);

            // Execute a word by found word information.
            void execute(const Word& word);

            // Execute a word by handler index.
            void execute(size_t word_index);

            // Execute a word by handler information.
            void execute(WordHandlerInfo& info);

        public:
            // Push a new call stack item onto the call stack.
            void call_stack_push(const WordHandlerInfo& info) noexcept;

            // Push a new call stack item onto the call stack.
            void call_stack_push(const std::string& name,
                                 const source::Location& location) noexcept;

            // Pop a call stack item from the call stack.
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
        runtime.add_word(name, \
                         std::filesystem::path(__FILE__), \
                         __LINE__, \
                         1, \
                         handler, \
                         sorth::compilation::WordExecutionContext::run_time, \
                         sorth::compilation::WordVisibility::visible, \
                         sorth::compilation::WordType::internal, \
                         sorth::compilation::WordContextManagement::unmanaged)


    // Called by C++ code to add a new immediate word to the compiler's runtime for access to other
    // immediate words as executed by the compiler.
    #define ADD_NATIVE_IMMEDIATE_WORD(runtime, name, handler) \
        runtime.add_word(name, \
                         __FILE__, \
                         __LINE__, \
                         1, \
                         handler, \
                         sorth::compilation::WordExecutionContext::compile_time, \
                         sorth::compilation::WordVisibility::visible, \
                         sorth::compilation::WordType::internal, \
                         sorth::compilation::WordContextManagement::unmanaged)


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


    class SearchPathManager
    {
        private:
            CompilerRuntime& runtime;

        public:
            SearchPathManager(CompilerRuntime& runtime, const std::filesystem::path& path)
            : runtime(runtime)
            {
                runtime.append_search_path(path);
            }

            SearchPathManager(const SearchPathManager& manager) = delete;
            SearchPathManager(SearchPathManager&& manager) = delete;

            ~SearchPathManager()
            {
                runtime.drop_search_path();
            }

        public:
            SearchPathManager& operator =(const SearchPathManager& manager) = delete;
            SearchPathManager& operator =(SearchPathManager&& manager) = delete;
    };


    class CompileContextManager
    {
        private:
            CompilerRuntime& runtime;

        public:
            CompileContextManager(CompilerRuntime& runtime, source::TokenList&& tokens)
            : runtime(runtime)
            {
                runtime.create_compile_context(std::move(tokens));
            }

            CompileContextManager(const CompileContextManager& manager) = delete;
            CompileContextManager(CompileContextManager&& manager) = delete;

            ~CompileContextManager()
            {
                runtime.drop_compile_context();
            }

        public:
            CompileContextManager& operator =(const CompileContextManager& manager) = delete;
            CompileContextManager& operator =(CompileContextManager&& manager) = delete;
    };


}

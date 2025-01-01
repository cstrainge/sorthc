
#pragma once



namespace sorth::compilation::byte_code
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
            // What type of code generation are we doing?
            enum class CodeGenType
            {
                // We are generating a word.
                word,

                // We are generating the top level of a script.
                script_body
            };

        private:
            // The llvm execution engine used for JITing code.
            std::unique_ptr<llvm::orc::LLJIT> jit = nullptr;

            llvm::Function* handle_set_location_fn = nullptr;
            llvm::Function* handle_manage_context_fn = nullptr;

            llvm::Function* handle_define_variable_fn = nullptr;
            llvm::Function* handle_define_constant_fn = nullptr;
            llvm::Function* handle_read_variable_fn = nullptr;
            llvm::Function* handle_write_variable_fn = nullptr;

            llvm::Function* handle_pop_bool_fn = nullptr;

            llvm::Function* handle_push_last_exception_fn = nullptr;
            llvm::Function* handle_push_bool_fn = nullptr;
            llvm::Function* handle_push_int_fn = nullptr;
            llvm::Function* handle_push_double_fn = nullptr;
            llvm::Function* handle_push_string_fn = nullptr;
            llvm::Function* handle_push_value_fn = nullptr;

            llvm::Function* handle_word_execute_name_fn = nullptr;
            llvm::Function* handle_word_execute_index_fn = nullptr;

            llvm::Function* handle_word_index_name_fn = nullptr;
            llvm::Function* handle_word_exists_name_fn = nullptr;

            // The storage for the last exception that occurred in the JITed code.
            static thread_local std::optional<std::runtime_error> last_exception;

        public:
            Jit();
            Jit(const Jit& jit) = delete;
            Jit(Jit&& jit) = delete;
            ~Jit() = default;

        public:
            Jit& operator =(const Jit& jit) = delete;
            Jit& operator =(Jit&& jit) = delete;

        public:
            run_time::WordHandler jit_compile(run_time::CompilerRuntime& runtime,
                                              const std::string& name,
                                              const ByteCode& code);

            // JIT compile a word construction into native code and register it for running within
            // the compiler's runtime.  This can be an immediate word or run-time word intended for
            // use by immediate words.
            void jit_compile(run_time::CompilerRuntime& runtime, const Construction& construction);

            // JIT compile a script into native code for running within the compiler's runtime.
            // Register all of the scripts non-immediate words for execution for other immediate
            // words.
            //
            // The immediate words would have already been compiled and registered with the
            // compiler's runtime.
            void jit_compile(run_time::CompilerRuntime& runtime, const ScriptPtr& script);

        private:
            // Register the helper function pointers with the JIT engine.
            void register_jit_helper_ptrs();

            // Register our helper functions with the JIT module being generated.
            void register_jit_helpers(std::unique_ptr<llvm::Module>& module,
                                      std::unique_ptr<llvm::LLVMContext>& context);

            std::string filter_word_name(const std::string& name);

        private:
            // Create a new JIT compilation module and context with the given name.
            std::tuple<std::unique_ptr<llvm::Module>,
                       std::unique_ptr<llvm::LLVMContext>>
                                                 create_jit_module_context(const std::string& name);

            std::unordered_map<std::string,
                               std::string>
                                      finalize_module(std::unique_ptr<llvm::Module>&& module,
                                                      std::unique_ptr<llvm::LLVMContext>&& context);

            // JIT compile the given byte-code block into a native function handler.
            std::tuple<std::vector<source::Location>,
                       std::vector<run_time::Value>>
                                       jit_compile_word(run_time::CompilerRuntime& interpreter,
                                                        std::unique_ptr<llvm::Module>& module,
                                                        std::unique_ptr<llvm::LLVMContext>& context,
                                                        const std::string& name,
                                                        const ByteCode& code,
                                                        CodeGenType type);

            run_time::WordHandler create_word_function(const std::string& name,
                                                       std::string&& function_ir,
                                                       std::vector<source::Location>&& locations_,
                                                       std::vector<run_time::Value>&& constants_,
                                                       CodeGenType type);

            // Generate the body of the JITed function.  We do this by walking the byte-code block
            // and generating the appropriate llvm instructions for each operation.
            void generate_function_body(run_time::CompilerRuntime& runtime,
                                        std::unique_ptr<llvm::Module>& module,
                                        llvm::IRBuilder<>& builder,
                                        llvm::Function* function,
                                        std::unique_ptr<llvm::LLVMContext>& context,
                                        const std::string& name,
                                        std::vector<source::Location>& locations,
                                        std::vector<run_time::Value>& constants,
                                        const ByteCode& code);

            // Define a new string constant with the given text in the JIT module.
            llvm::Value* define_string_constant(const std::string& text,
                                                llvm::IRBuilder<>& builder,
                                                std::unique_ptr<llvm::Module>& module,
                                                std::unique_ptr<llvm::LLVMContext>& context);

        private:
            static void handle_set_location(void* runtime_ptr,
                                            void* location_array_ptr,
                                            size_t index);
            static void handle_manage_context(void* runtime_ptr, bool is_marking);
            static int64_t handle_define_variable(void* runtime_ptr, const char* name);
            static int64_t handle_define_constant(void* runtime_ptr, const char* name);
            static int64_t handle_read_variable(void* runtime_ptr);
            static int64_t handle_write_variable(void* runtime_ptr);
            static void handle_push_last_exception(void* runtime_ptr);
            static int64_t handle_pop_bool(void* interpreter_ptr, bool* value);
            static void handle_push_bool(void* runtime_ptr, bool value);
            static void handle_push_int(void* runtime_ptr, int64_t value);
            static void handle_push_double(void* runtime_ptr, double value);
            static void handle_push_string(void* runtime_ptr, const char* value);
            static void handle_push_value(void* runtime_ptr, void* array_ptr, int64_t index);
            static int64_t handle_word_execute_name(void* runtime_ptr, const char* name);
            static int64_t handle_word_execute_index(void* runtime_ptr, int64_t index);
            static int64_t handle_word_index_name(void* runtime_ptr, const char* name);
            static int64_t handle_word_exists_name(void* runtime_ptr, const char* name);

        private:
            // Set the last exception that occurred.
            static void set_last_exception(const std::runtime_error& error);

            // Clear the last exception that occurred.
            static void clear_last_exception();
    };


    // Get the instance of the JIT engine for the compiler's runtime.
    Jit& get_jit_engine();


}

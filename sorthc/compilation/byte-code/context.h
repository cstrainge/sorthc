
#pragma once



namespace sorth::compilation::run_time
{


    class CompilerRuntime;


}



namespace sorth::compilation::byte_code
{


    struct FfiFunction
    {
        std::string name;
        std::string alias;
        std::vector<std::string> argument_types;
        std::string return_type;
    };


    using FfiFunctionList = std::vector<FfiFunction>;


    // Where in the bytecode list should new code be inserted?
    enum class CodeInsertionPoint
    {
        // Insert the new code at the beginning of the list.
        beginning,

        // Insert the new code at the end of the list.
        end
    };


    // This class represents the context of the byte-code compiler.  Each contexts represents a
    // single script.  It holds the tokens that make up that script and manages the compilation
    // process for that script.
    class Context
    {
        private:
            // Reference to the owning compiler run-time.
            run_time::CompilerRuntime& runtime;

            // List of scripts that have been loaded by the one being constructed.
            SubScriptList sub_scripts;

            // Stack of constructions that are being managed by the compile context.  A construction
            // can represent a word or a top level block of code being constructed.
            ConstructionStack constructions;

            // List of words that have been defined in the script.
            ConstructionList words;

            // The list of structure types that have been defined in the script.
            StructureTypeList structure_types;

            // The list of FFI functions that have been defined in the script.
            FfiFunctionList ffi_functions;

            // Where in the top construction's code the next instructions should be inserted.
            CodeInsertionPoint insertion_point;

            // Tokens that make up the context that we're about to compile.
            source::TokenList tokens;

            // The index of the current token to be compiled.
            size_t current_token;

        public:
            // Create a new context associated with a given runtime and take ownership of the given
            // tokens for compilation.
            Context(run_time::CompilerRuntime& runtime, source::TokenList&& tokens) noexcept;
            Context(const Context& context) = delete;
            Context(Context&& context) = delete;
            ~Context() noexcept = default;

        public:
            Context& operator =(const Context& context) = delete;
            Context& operator =(Context&& context) = delete;

        public:
            const ConstructionList& get_words() const noexcept;

            ConstructionList&& take_words() noexcept;

        public:
            void add_script_word(const Construction& word);

            void add_script_structure(const StructureType& structure);

            void add_ffi_function(const FfiFunction& function);

            const SubScriptList& get_sub_scripts() const noexcept;
            SubScriptList&& take_sub_scripts() noexcept;

            const StructureTypeList& get_structure_types() const noexcept;
            StructureTypeList&& take_structure_types() noexcept;

            const FfiFunctionList& get_ffi_functions() const noexcept;
            FfiFunctionList&& take_ffi_functions() noexcept;

        public:
            // Compile the context into byte-code instructions.
            void compile_token_list();

            // Compile a single token into byte-code instruction(s).
            void compile_token(const source::Token& token);

            // Compile tokens until a given word token is found.
            std::string compile_until_words(const std::vector<std::string>& words);

        public:
            // Attempt to get the next token in the context's token stream.
            const source::Token& get_next_token();

        public:
            void append_script(const byte_code::ScriptPtr& script);

        public:
            // Create a new construction an push it onto the stack.
            void new_construction() noexcept;

            // Create a new construction with the given name and location and push it onto the
            // stack.
            void new_construction(const source::Location& location,
                                  const std::string& name) noexcept;

            // Create a new unnamed construction with the given byte-code and push it onto the
            // stack.
            void new_construction(ByteCode&& code) noexcept;


            // Pop the top construction off the stack and return it.
            Construction drop_construction();


            // Merge the top construction on the stack into the one below it.
            void merge_constructions();


            // Get the top construction on the stack.
            Construction& get_construction();

        public:
            // Set where in the top construction's code the next instructions should be inserted.
            void set_insertion_point(CodeInsertionPoint point) noexcept;

            // Insert an instruction into the top construction's code at the current insertion
            // point.
            void insert_instruction(const Instruction& instruction);

            // Insert an instruction into the top construction's code at the current insertion
            // point.
            void insert_instruction(const source::Location& location,
                                    Instruction::Id id,
                                    const run_time::Value& value);

            // Insert an instruction into the top construction's code at the current insertion
            // point.
            void insert_instruction(Instruction::Id id, const run_time::Value& value);

            // Insert an instruction into the top construction's code at the current insertion
            // point.
            void insert_instruction(Instruction::Id id);
    };


    using ContextStack = std::stack<Context*>;


}

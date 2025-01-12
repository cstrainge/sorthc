
#pragma once



namespace sorth::compilation::byte_code
{


    class Construction;
    using ConstructionList = std::vector<Construction>;

    class Instruction;
    using ByteCode = std::vector<Instruction>;


    class Script;

    using ScriptPtr = std::shared_ptr<Script>;
    using SubScriptList = std::vector<ScriptPtr>;


    using WordMap = std::unordered_map<std::string, size_t>;



    struct FfiFunction;
    using FfiFunctionList = std::vector<FfiFunction>;

    struct FfiVariable;
    using FfiVariableList = std::vector<FfiVariable>;

    struct StructureType;

    struct FfiArrayType;

    using TypeInfo = std::variant<StructureType, FfiArrayType>;
    using DataTypeList = std::vector<TypeInfo>;


    class Script : public std::enable_shared_from_this<Script>
    {
        private:
            // The list of sub-scripts that have been loaded by the script.
            SubScriptList sub_scripts;

            // The path to the original script file.
            std::filesystem::path script_path;

            // The list of words that have been defined in the script.
            ConstructionList words;

            // The list of structure types that have been defined in the script.
            DataTypeList data_types;

            // The list of FFI functions that have been defined in the script.
            FfiFunctionList ffi_functions;

            // The list of FFI variables that have been defined in the script.
            FfiVariableList ffi_variables;

            // A map of word names to their index in the words list.
            WordMap word_map;

            // The list of instructions that make up the top level of the script.
            ByteCode top_level;

        public:
            Script() = default;
            Script(SubScriptList&& sub_scripts,
                   std::filesystem::path&& script_path,
                   ConstructionList&& words,
                   DataTypeList&& data_types,
                   FfiFunctionList&& ffi_functions,
                   FfiVariableList&& ffi_variables,
                   ByteCode&& top_level) noexcept;
            Script(const Script& script) = default;
            Script(Script&& script) = default;
            ~Script() noexcept = default;

        public:
            Script& operator =(const Script& script) = default;
            Script& operator =(Script&& script) = default;

        public:
            const SubScriptList& get_sub_scripts() const noexcept;

            const std::filesystem::path& get_script_path() const noexcept;

            const ConstructionList& get_words() const noexcept;
            const Construction& get_word(const std::string& name) const;

            const DataTypeList& get_data_types() const noexcept;

            const FfiFunctionList& get_ffi_functions() const noexcept;

            const FfiVariableList& get_ffi_variables() const noexcept;

            const ByteCode& get_top_level() const noexcept;
    };


}

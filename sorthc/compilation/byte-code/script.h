
#pragma once



namespace sorthc::compilation::byte_code
{


    class Construction;
    using ConstructionList = std::vector<Construction>;

    class Instruction;
    using ByteCode = std::vector<Instruction>;


    class Script;

    using ScriptPtr = std::shared_ptr<Script>;
    using SubScriptList = std::vector<ScriptPtr>;


    using WordMap = std::unordered_map<std::string, size_t>;


    class StructureType;
    using StructureTypeList = std::vector<StructureType>;


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
            StructureTypeList structure_types;

            // A map of word names to their index in the words list.
            WordMap word_map;

            // The list of instructions that make up the top level of the script.
            ByteCode top_level;

        public:
            Script() = default;
            Script(SubScriptList&& sub_scripts,
                   std::filesystem::path&& script_path,
                   ConstructionList&& words,
                   StructureTypeList&& structure_types,
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

            const StructureTypeList& get_structure_types() const noexcept;

            const ByteCode& get_top_level() const noexcept;
    };


}

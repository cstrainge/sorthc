
#pragma once



namespace sorthc::compilation::byte_code
{


    class Construction;
    using ConstructionList = std::vector<Construction>;

    class Instruction;
    using ByteCode = std::vector<Instruction>;


    class Script;
    using SubScriptList = std::vector<Script>;


    using WordMap = std::unordered_map<std::string, size_t>;


    class Script : public std::enable_shared_from_this<Script>
    {
        private:
            // The path to the original script file.
            std::filesystem::path script_path;

            // The list of words that have been defined in the script.
            ConstructionList words;

            // A map of word names to their index in the words list.
            WordMap word_map;

            // The list of instructions that make up the top level of the script.
            ByteCode top_level;

            // The list of sub-scripts that have been loaded by the script.
            SubScriptList sub_scripts;

        public:
            Script() = default;
            Script(std::filesystem::path&& script_path,
                   ConstructionList&& words,
                   ByteCode&& top_level) noexcept;
            Script(const Script& script) = default;
            Script(Script&& script) = default;
            ~Script() noexcept = default;

        public:
            Script& operator =(const Script& script) = default;
            Script& operator =(Script&& script) = default;

        public:
            const std::filesystem::path& get_script_path() const noexcept;

            const ConstructionList& get_words() const noexcept;
            const Construction& get_word(const std::string& name) const;

            const ByteCode& get_top_level() const noexcept;
    };


}

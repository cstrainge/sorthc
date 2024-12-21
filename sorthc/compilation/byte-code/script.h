
#pragma once



namespace sorthc::compilation::byte_code
{


    class Construction;
    using ConstructionList = std::vector<Construction>;

    class Instruction;
    using ByteCode = std::vector<Instruction>;


    using WordCollection = std::unordered_map<std::string, Construction>;


    class Script
    {
        private:
            // The path to the original script file.
            std::filesystem::path script_path;

            // The list of words that have been defined in the script.
            WordCollection words;

            // The list of instructions that make up the top level of the script.
            ByteCode top_level;

        public:
            Script() = default;
            Script(std::filesystem::path&& script_path,
                   const ConstructionList& words,
                   ByteCode&& top_level) noexcept;
            Script(const Script& script) = default;
            Script(Script&& script) = default;
            ~Script() noexcept = default;

        public:
            Script& operator =(const Script& script) = default;
            Script& operator =(Script&& script) = default;

        public:
            const std::filesystem::path& get_script_path() const noexcept;
            const Construction& get_word(const std::string& name) const;
            const WordCollection& get_words() const noexcept;
            const ByteCode& get_top_level() const noexcept;

        private:
            WordCollection convert_word_list(const ConstructionList& words);
    };


}


#pragma once



namespace sorth::compilation::byte_code
{


    class Instruction;
    using ByteCode = std::vector<Instruction>;



    // Information about a word or top level code for a script as it's being constructed.  A
    // stack of these constructions is managed by the compile context to manage creating new
    // words and code blocks.
    //
    // When the block of code is the top level of the script, most of these fields are not used.
    // However when it's a new word being defined, all of these fields are used.
    class Construction
    {
        private:
            WordExecutionContext execution_context;
            WordVisibility visibility;
            WordContextManagement context_management;

            source::Location location;

            std::string name;
            std::string description;
            std::string signature;

            ByteCode code;

        public:
            Construction() noexcept;
            Construction(const source::Location& location, const std::string& name) noexcept;
            Construction(const ByteCode&& code) noexcept;
            Construction(const source::Location& location,
                         const std::string& name,
                         const ByteCode&& code) noexcept;
            Construction(const Construction& other) noexcept = default;
            Construction(Construction&& other) noexcept = default;

        public:
            Construction& operator =(const Construction& other) noexcept = default;
            Construction& operator =(Construction&& other) noexcept = default;

        public:
            WordExecutionContext get_execution_context() const noexcept;
            void set_execution_context(WordExecutionContext value) noexcept;

            WordVisibility get_visibility() const noexcept;
            void set_visibility(WordVisibility value) noexcept;

            WordContextManagement get_context_management() const noexcept;
            void set_context_management(WordContextManagement value) noexcept;

            const source::Location& get_location() const noexcept;
            void set_location(const source::Location& value) noexcept;

            const std::string& get_name() const noexcept;
            void set_name(const std::string& value) noexcept;

            const std::string& get_description() const noexcept;
            void set_description(const std::string& value) noexcept;

            const std::string& get_signature() const noexcept;
            void set_signature(const std::string& value) noexcept;

            ByteCode& get_code() noexcept;
            const ByteCode& get_code() const noexcept;
            ByteCode&& take_code() noexcept;

            void set_code(const ByteCode& value) noexcept;
            void set_code(ByteCode&& value) noexcept;
    };


    // Stack of constructions that are being managed by the compile context.
    using ConstructionStack = std::stack<Construction>;


    // List of constructions that represents words that have been defined in the script.
    using ConstructionList = std::vector<Construction>;


}

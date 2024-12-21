
#pragma once



namespace sorthc::compilation
{


    // In what context should this word be executed?
    enum class WordExecutionContext
    {
        // Execute this word at compile time.
        compile_time,

        // Execute this word at run time.
        run_time
    };


    // What type of word is this?
    enum class WordType
    {
        // A word that is defined in Forth script.
        scripted,

        // A word that is defined in the interpreter.
        internal
    };


    // Should this word be visible in the dictionary?
    enum class WordVisibility
    {
        // The word is visible in the dictionary.
        visible,

        // The word is hidden from the dictionary.
        hidden
    };


    // How should this word's variables be managed?
    enum class WordContextManagement
    {
        // The word's variables are managed by the interpreter.
        managed,

        // The word's variables are managed by the word itself.
        unmanaged
    };


    // Represents a word in the Strange Forth compiler.
    class Word
    {
        private:
            WordExecutionContext context;
            WordType type;
            WordVisibility visibility;
            WordContextManagement management;

            std::string name;
            source::Location location;

            size_t handler_index;

        public:
            Word() noexcept = default;
            Word(WordExecutionContext context,
                 WordType type,
                 WordVisibility visibility,
                 WordContextManagement management,
                 const std::string& name,
                 const source::Location& location,
                 size_t handler_index) noexcept;
            Word(const Word& word) noexcept = default;
            Word(Word&& word) noexcept = default;
            ~Word() noexcept = default;

        public:
            Word& operator =(const Word& word) noexcept = default;
            Word& operator =(Word&& word) noexcept = default;

        public:
            WordExecutionContext get_context() const noexcept;
            WordType get_type() const noexcept;
            WordVisibility get_visibility() const noexcept;
            WordContextManagement get_management() const noexcept;

            const std::string& get_name() const noexcept;
            const source::Location& get_location() const noexcept;

            size_t get_handler_index() const noexcept;
    };


}


#pragma once



namespace sorthc::compilation::run_time
{


    // The Forth dictionary.  Handlers for Forth words are not stored directly in the dictionary.
    // Instead they are stored in their own list and the index and any important flags are what is
    // stored in the dictionary directly.
    //
    // Also note that the dictionary is implemented as a stack of dictionaries.  This allows the
    // Forth to contain scopes.  If a word is redefined in a higher scope, it effectively replaces
    // that word until that scope is released.
    class Dictionary
    {
        private:
            using SubDictionary = std::unordered_map<std::string, Word>;
            using DictionaryStack = std::list<SubDictionary>;

            DictionaryStack stack;

        public:
            Dictionary();
            Dictionary(const Dictionary& dictionary);

        public:
            void insert(const std::string& text, const Word& value);
            std::tuple<bool, Word> find(const std::string& word) const;

        public:
            void mark_context();
            void release_context();

        protected:
            friend std::ostream& operator <<(std::ostream& stream, const Dictionary& dictionary);
    };


}

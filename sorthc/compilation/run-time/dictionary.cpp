
#include "sorthc.h"



namespace sorth::compilation::run_time
{


    Dictionary::Dictionary()
    {
        // Start with an empty dictionary.
        mark_context();
    }


    Dictionary::Dictionary(const Dictionary& dictionary)
    {
        // start with the empty dictionary.
        mark_context();

        // Now merge all the sub dictionaries into this one.
        for (auto iter = dictionary.stack.rbegin(); iter != dictionary.stack.rend(); ++iter)
        {
            stack.front().insert(iter->begin(), iter->end());
        }
    }


    void Dictionary::insert(const std::string& text, const Word& value)
    {
        auto& current_dictionary = stack.front();
        auto iter = current_dictionary.find(text);

        if (iter != current_dictionary.end())
        {
            iter->second = value;
        }
        else
        {
            current_dictionary.insert(SubDictionary::value_type(text, value));
        }
    }


    std::tuple<bool, Word> Dictionary::find(const std::string& word) const
    {
        for (auto stack_iter = stack.begin(); stack_iter != stack.end(); ++stack_iter)
        {
            auto iter = stack_iter->find(word);

            if (iter != stack_iter->end())
            {
                return { true, iter->second };
            }
        }

        return { false, {} };
    }


    void Dictionary::mark_context()
    {
        // Create a new dictionary.
        stack.push_front({});
    }


    void Dictionary::release_context()
    {
        stack.pop_front();

        // There should always be at least one dictionary.  If there isn't something has
        // gone horribly wrong.
        assert(!stack.empty());
    }


}

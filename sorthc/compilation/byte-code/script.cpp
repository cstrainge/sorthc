
#include "sorthc.h"



namespace sorthc::compilation::byte_code
{


    Script::Script(std::filesystem::path&& script_path,
                   const ConstructionList& words,
                   ByteCode&& top_level) noexcept
    : script_path(std::move(script_path)),
      words(convert_word_list(words)),
      top_level(std::move(top_level))
    {
    }


    const std::filesystem::path& Script::get_script_path() const noexcept
    {
        return script_path;
    }


    const Construction& Script::get_word(const std::string& name) const
    {
        const auto& found = words.find(name);

        if (found == words.end())
        {
            throw_error("Word " + name + " not found in script.");
        }

        return found->second;
    }


    const WordCollection& Script::get_words() const noexcept
    {
        return words;
    }


    const ByteCode& Script::get_top_level() const noexcept
    {
        return top_level;
    }


    WordCollection Script::convert_word_list(const ConstructionList& constructions)
    {
        WordCollection collection;

        for (const auto& construction : constructions)
        {
            collection.insert(std::make_pair(construction.get_name(), construction));
        }

        return collection;
    }


}

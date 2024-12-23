
#include "sorthc.h"



namespace sorthc::compilation::byte_code
{


    Script::Script(std::filesystem::path&& script_path,
                   ConstructionList&& words,
                   ByteCode&& top_level) noexcept
    : script_path(std::move(script_path)),
      words(std::move(words)),
      top_level(std::move(top_level)),
      word_map()
    {
        for (size_t i = 0; i < this->words.size(); ++i)
        {
            word_map.insert({ this->words[i].get_name(), i });
        }
    }


    const std::filesystem::path& Script::get_script_path() const noexcept
    {
        return script_path;
    }


    const ConstructionList& Script::get_words() const noexcept
    {
        return words;
    }


    const Construction& Script::get_word(const std::string& name) const
    {
        const auto& found = word_map.find(name);

        if (found == word_map.end())
        {
            throw_error("Word " + name + " not found in script.");
        }

        return words[found->second];
    }


    const ByteCode& Script::get_top_level() const noexcept
    {
        return top_level;
    }


}

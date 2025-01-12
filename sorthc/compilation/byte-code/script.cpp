
#include "sorthc.h"



namespace sorth::compilation::byte_code
{


    Script::Script(SubScriptList&& sub_scripts,
                   std::filesystem::path&& script_path,
                   ConstructionList&& words,
                   DataTypeList&& data_types,
                   FfiFunctionList&& ffi_functions,
                   FfiVariableList&& ffi_variables,
                   ByteCode&& top_level) noexcept
    : sub_scripts(std::move(sub_scripts)),
      script_path(std::move(script_path)),
      words(std::move(words)),
      data_types(std::move(data_types)),
      ffi_functions(std::move(ffi_functions)),
      ffi_variables(std::move(ffi_variables)),
      top_level(std::move(top_level)),
      word_map()
    {
        for (size_t i = 0; i < this->words.size(); ++i)
        {
            word_map.insert({ this->words[i].get_name(), i });
        }
    }


    const SubScriptList& Script::get_sub_scripts() const noexcept
    {
        return sub_scripts;
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


    const DataTypeList& Script::get_data_types() const noexcept
    {
        return data_types;
    }


    const FfiFunctionList& Script::get_ffi_functions() const noexcept
    {
        return ffi_functions;
    }


    const FfiVariableList& Script::get_ffi_variables() const noexcept
    {
        return ffi_variables;
    }


    const ByteCode& Script::get_top_level() const noexcept
    {
        return top_level;
    }


}

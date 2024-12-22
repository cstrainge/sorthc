
#include "sorthc.h"



namespace sorthc::compilation::run_time
{


    CompilerRuntime::CompilerRuntime(const std::filesystem::path& system_path)
    : search_paths(),
      script_cache(),
      location(LOCATION_HERE),
      compile_contexts()
    {
        // Append the system path to the search paths.
        append_search_path(system_path);

        // Initialize the dictionary with our native words known to the compiler run-time.  Then
        // compile the standard library.
        init_dictionary();

        // Note that we only load a subset of the standard library here, the rest of it is loaded
        // when the compilation phase is run.
        //
        // The core words are shared by the compiler's internal runtime and the run-time that the
        // compiled code ultimately runs in.
        auto& std_lib = compile_script("std/core-words.f");

        // JIT compile the standard library's words and top level code, the words in the script will
        // automaticaly be added to the run-time's dictionary.
        auto top_level = byte_code::get_jit_engine().jit_compile(*this, std_lib);
        top_level(*this);
    }


    const source::Location& CompilerRuntime::get_location() const noexcept
    {
        return location;
    }


    void CompilerRuntime::set_location(const source::Location& value) noexcept
    {
        location = value;
    }


    const ScriptCache& CompilerRuntime::get_script_cache() const noexcept
    {
        return script_cache;
    }


    void CompilerRuntime::mark_context()
    {
        dictionary.mark_context();
        handlers.mark_context();
        variables.mark_context();
    }


    void CompilerRuntime::release_context()
    {
        dictionary.release_context();
        handlers.release_context();
        variables.release_context();
    }


    void CompilerRuntime::append_search_path(const std::filesystem::path& path)
    {
        // TODO: Normalize the path, and make sure it actually exists and is a directory.
        search_paths.push_back(path);
    }


    // Drop the last search path from the run-time's search path collection.
    void CompilerRuntime::drop_search_path()
    {
        throw_error_if(search_paths.empty(), *this, "No search paths to drop.");

        search_paths.pop_back();
    }


    // Compile a script to byte-code and add it to the cache.
    byte_code::Script& CompilerRuntime::compile_script(const std::filesystem::path& path)
    {
        // First check if the script is already in the cache, if it's already there we con't need to
        // do anything.  Note that we still do a full find_file on it to make sure the path is
        // normalized.  We don't want to load the same script multiple times through different
        // relative paths.
        auto full_path = find_file(path);
        auto iter = script_cache.find(full_path);

        if (iter != script_cache.end())
        {
            return iter->second;
        }

        // Get the base of the file's path so we can use it to search for included files by that
        // script.
        auto base_path = full_path;
        base_path.remove_filename();

        // Append the path to the search path temporarily.
        auto search_path_manager = SearchPathManager(*this, base_path);

        // Read the source file into a buffer and tokenize it.
        auto buffer = source::SourceBuffer(full_path);
        auto tokens = source::tokenize_source(buffer);

        // Create a new compile context and compile the tokens into byte-code.
        auto compile_context_manager = CompileContextManager(*this, std::move(tokens));

        get_compile_context().compile_token_list();

        // Create a new script object and add it to the cache.
        auto construction = get_compile_context().drop_construction();
        auto& words = get_compile_context().get_words();
        auto code = std::move(construction.take_code());

        auto script = byte_code::Script(std::move(full_path), words, std::move(code));

        // Cache the script, and return the reference to the cached one.
        script_cache.insert(std::make_pair(script.get_script_path(), script));

        return script_cache[script.get_script_path()];
    }


    // Construct a new compilation context for the given token list representing a script.
    void CompilerRuntime::create_compile_context(source::TokenList&& tokens)
    {
        compile_contexts.push(new byte_code::Context(*this, std::move(tokens)));
    }


    // Drop the current compile context after finishing the compilation of a script file.
    void CompilerRuntime::drop_compile_context()
    {
        throw_error_if(compile_contexts.empty(), *this, "No compile context to drop.");

        delete compile_contexts.top();
        compile_contexts.pop();
    }


    // Get the current compile context.
    byte_code::Context& CompilerRuntime::get_compile_context()
    {
        throw_error_if(compile_contexts.empty(), *this, "No compile context to access.");

        return *compile_contexts.top();
    }


    void CompilerRuntime::add_word(const WordHandlerInfo& info,
                                   WordExecutionContext execution_context,
                                   WordVisibility visibility,
                                   WordType type,
                                   WordContextManagement context_management)
    {
        std::cout << "Adding word: " << info.name << std::endl;
        dictionary.insert(info.name,
                          Word(execution_context,
                               type,
                               visibility,
                               context_management,
                               info.name,
                               info.location,
                               handlers.insert(info)));
    }


    void CompilerRuntime::add_word(const std::string& name,
                                   const source::Location& location,
                                   WordHandler handler,
                                   WordExecutionContext execution_context,
                                   WordVisibility visibility,
                                   WordType type,
                                   WordContextManagement context_management)
    {
        add_word({ .location = location, .name = name, .handler = handler },
                 execution_context,
                 visibility,
                 type,
                 context_management);
    }


    void CompilerRuntime::add_word(const std::string& name,
                                   const std::filesystem::path& path,
                                   size_t line,
                                   size_t column,
                                   WordHandler handler,
                                   WordExecutionContext execution_context,
                                   WordVisibility visibility,
                                   WordType type,
                                   WordContextManagement context_management)
    {
        add_word(name,
                 source::Location(path.string(), line, column),
                 handler,
                 execution_context,
                 visibility,
                 type,
                 context_management);
    }


    const std::tuple<bool, Word> CompilerRuntime::find(const std::string& word_name) const noexcept
    {
        return dictionary.find(word_name);
    }


    const std::tuple<bool, WordHandlerInfo>
                                          CompilerRuntime::find(size_t handler_index) const noexcept
    {
        if (handler_index >= handlers.size())
        {
            return { false, {} };
        }

        auto info = handlers[handler_index];

        return { true, info };
    }


    void CompilerRuntime::define_variable(const std::string& name) noexcept
    {
        auto index = variables.insert(Value());
        auto handler = [index](CompilerRuntime& runtime)
            {
                runtime.push(static_cast<int64_t>(index));
            };

        auto& this_ref = *this;
        ADD_NATIVE_WORD(this_ref, name, handler);
    }


    void CompilerRuntime::define_constant(const std::string& name, const Value& value) noexcept
    {
        auto handler = [value](CompilerRuntime& runtime)
            {
                runtime.push(value);
            };

        auto& this_ref = *this;
        ADD_NATIVE_WORD(this_ref, name, handler);
    }


    Value CompilerRuntime::read_variable(size_t index)
    {
        throw_error_if(index >= variables.size(), *this, "Variable index out of range.");

        return variables[index];
    }


    void CompilerRuntime::write_variable(size_t index, Value value)
    {
        throw_error_if(index >= variables.size(), *this, "Variable index out of range.");

        variables[index] = value;
    }


    void CompilerRuntime::push(const Value& value) noexcept
    {
        data_stack.push_back(value);
    }


    Value CompilerRuntime::pop()
    {
        throw_error_if(data_stack.empty(), *this, "Data stack underflow.");

        auto value = data_stack.back();
        data_stack.pop_back();

        return value;
    }


    int64_t CompilerRuntime::pop_as_integer()
    {
        return pop().get_int(*this);
    }


    size_t CompilerRuntime::pop_as_size()
    {
        return static_cast<size_t>(pop_as_integer());
    }


    double CompilerRuntime::pop_as_float()
    {
        return pop().get_double(*this);
    }


    bool CompilerRuntime::pop_as_bool()
    {
        return pop().get_bool(*this);
    }


    std::string CompilerRuntime::pop_as_string()
    {
        return pop().get_string(*this);
    }


    byte_code::ByteCode CompilerRuntime::pop_as_byte_code()
    {
        return pop().get_byte_code();
    }


    void CompilerRuntime::init_dictionary()
    {
        built_in_words::register_words(*this);
    }


    void CompilerRuntime::execute(const std::string& word)
    {
        auto [ found, word_info ] = find(word);

        throw_error_if(!found, *this, "Word " + word + " not found.");

        execute(word_info);
    }


    void CompilerRuntime::execute(const Word& word)
    {
        auto [ found, handler_info ] = find(word.get_handler_index());

        throw_error_if(!found, *this, "Handler for word " + word.get_name() + " not found.");

        execute(handler_info);
    }


    void CompilerRuntime::execute(size_t word_index)
    {
        auto [ found, handler_info ] = find(word_index);

        throw_error_if(!found,
                       *this,
                       "Handler for word index " + std::to_string(word_index) + " not found.");

        execute(handler_info);
    }


    void CompilerRuntime::execute(const WordHandlerInfo& info)
    {
        CallStackManager manager(*this, info);

        info.handler(*this);
    }


    void CompilerRuntime::call_stack_push(const WordHandlerInfo& info) noexcept
    {
        call_stack_push(info.name, info.location);
    }


    void CompilerRuntime::call_stack_push(const std::string& name,
                                          const source::Location& location) noexcept
    {
        call_stack.push({ .location = location, .name = name });
    }


    void CompilerRuntime::call_stack_pop()
    {
        throw_error_if(call_stack.empty(), *this, "Call stack underflow.");

        call_stack.pop();
    }


    std::filesystem::path CompilerRuntime::find_file(const std::filesystem::path& path)
    {
        // If the path is absolute, we don't need to search for it.
        if (path.is_absolute())
        {
            return path;
        }

        // Search for the file in the search paths.
        for (const auto& search_path : search_paths)
        {
            auto full_path = search_path / path;

            if (std::filesystem::exists(full_path))
            {
                return full_path;
            }
        }

        // If we didn't find the file, throw an error.
        throw_error(*this, "Could not find file " + path.string() + ".");
    }


}

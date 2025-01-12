
#include "sorthc.h"



namespace sorth::compilation::run_time
{


    CompilerRuntime::CompilerRuntime(const std::filesystem::path& system_path)
    : standard_library(),
      search_paths(),
      script_cache(),
      location(LOCATION_HERE),
      compile_contexts()
    {
        is_building_runtime = true;

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
        auto runtime_lib = compile_script("std/compiler-std.f");

        is_building_runtime = false;

        // JIT compile the core standard library's words and top level code, the words in the script
        // will automaticaly be added to the run-time's dictionary.
        //byte_code::get_jit_engine().jit_compile(*this, runtime_lib);

        // Now that we've JIT compiled the essential words, we can load the rest of the standard
        // library for later reference by the user script(s) we're going to compile.
        standard_library = compile_script("std.f");
    }


    bool CompilerRuntime::get_is_building_runtime() const noexcept
    {
        return is_building_runtime;
    }


    const byte_code::ScriptPtr& CompilerRuntime::get_standard_library() const noexcept
    {
        return standard_library;
    }


    const source::Location& CompilerRuntime::get_location() const noexcept
    {
        return location;
    }


    void CompilerRuntime::set_location(const source::Location& value) noexcept
    {
        location = value;
    }


    const CallStack& CompilerRuntime::get_call_stack() const noexcept
    {
        return call_stack;
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
    byte_code::ScriptPtr CompilerRuntime::compile_script(const std::filesystem::path& path)
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

        auto sub_scripts = std::move(get_compile_context().take_sub_scripts());
        auto words = std::move(get_compile_context().take_words());
        auto data_types = std::move(get_compile_context().take_types());
        auto ffi_functions = std::move(get_compile_context().take_ffi_functions());
        auto ffi_variables = std::move(get_compile_context().take_ffi_variables());
        auto code = std::move(construction.take_code());

        auto script = std::make_shared<byte_code::Script>(std::move(sub_scripts),
                                                          std::move(full_path),
                                                          std::move(words),
                                                          std::move(data_types),
                                                          std::move(ffi_functions),
                                                          std::move(ffi_variables),
                                                          std::move(code));

        // Cache the script, and return a reference to the new script.
        script_cache.insert(std::make_pair(script->get_script_path(), script));

        return script;
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
                                   WordContextManagement context_management,
                                   std::optional<byte_code::ByteCode>& code)
    {
        add_word({ .location = location, .name = name, .code = code, .handler = handler },
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
                                   WordContextManagement context_management,
                                   std::optional<byte_code::ByteCode>& code)
    {
        add_word(name,
                 source::Location(path.string(), line, column),
                 handler,
                 execution_context,
                 visibility,
                 type,
                 context_management,
                 code);
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
        std::optional<byte_code::ByteCode> code;

        add_word(name,
                 path,
                 line,
                 column,
                 handler,
                 execution_context,
                 visibility,
                 type,
                 context_management,
                 code);
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


    ArrayPtr CompilerRuntime::pop_as_array()
    {
        return pop().get_array(*this);
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

        throw_error_if(!found, *this, "Word " + stringify(word) + " not found.");

        execute(word_info);
    }


    void CompilerRuntime::execute(const Word& word)
    {
        auto [ found, handler_info ] = find(word.get_handler_index());

        throw_error_if(!found, *this, "Handler for word " + stringify(word.get_name())
                                                          + " not found.");

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


    void CompilerRuntime::execute(WordHandlerInfo& info)
    {
        CallStackManager manager(*this, info);

        if (   (info.code.has_value())
            && (!info.handler))
        {
            auto& jit = byte_code::get_jit_engine();

            info.handler = jit.jit_compile(*this, info.name, info.code.value());
            info.code.reset();
        }

        info.handler(*this);
    }


    void CompilerRuntime::call_stack_push(const WordHandlerInfo& info) noexcept
    {
        call_stack_push(info.name, info.location);
    }


    void CompilerRuntime::call_stack_push(const std::string& name,
                                          const source::Location& location) noexcept
    {
        call_stack.push_front({ .location = location, .name = name });
    }


    void CompilerRuntime::call_stack_pop()
    {
        throw_error_if(call_stack.empty(), *this, "Call stack underflow.");

        call_stack.pop_front();
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


#include "sorthc.h"



namespace sorth::compilation::byte_code
{


    Context::Context(run_time::CompilerRuntime& runtime, source::TokenList&& tokens) noexcept
    : runtime(runtime),
      sub_scripts(),
      constructions(),
      words(),
      data_types(),
      ffi_functions(),
      ffi_variables(),
      insertion_point(CodeInsertionPoint::end),
      tokens(std::move(tokens)),
      current_token(0)
    {
        // Create a new construction for the top level code.
        new_construction();
    }


    // Get the list of words that have been defined in the script.
    const ConstructionList& Context::get_words() const noexcept
    {
        return words;
    }


    ConstructionList&& Context::take_words() noexcept
    {
        return std::move(words);
    }


    // Compile the context into byte-code instructions.
    void Context::compile_token_list()
    {
        for (current_token = 0; current_token < tokens.size(); ++current_token)
        {
            compile_token(tokens[current_token]);
        }
    }


    void Context::add_script_word(const Construction& word)
    {
        words.push_back(word);
    }


    void Context::add_script_structure(const StructureType& structure)
    {
        data_types.push_back(structure);
    }


    void Context::add_ffi_function(const FfiFunction& function)
    {
        ffi_functions.push_back(function);
    }


    void Context::add_ffi_variable(const FfiVariable& variable)
    {
        ffi_variables.push_back(variable);
    }


    void Context::add_ffi_array_type(const FfiArrayType& array_type)
    {
        data_types.push_back(array_type);
    }


    const SubScriptList& Context::get_sub_scripts() const noexcept
    {
        return sub_scripts;
    }


    SubScriptList&& Context::take_sub_scripts() noexcept
    {
        return std::move(sub_scripts);
    }


    const FfiFunctionList& Context::get_ffi_functions() const noexcept
    {
        return ffi_functions;
    }


    FfiFunctionList&& Context::take_ffi_functions() noexcept
    {
        return std::move(ffi_functions);
    }


    const FfiVariableList& Context::get_ffi_variables() const noexcept
    {
        return ffi_variables;
    }


    FfiVariableList&& Context::take_ffi_variables() noexcept
    {
        return std::move(ffi_variables);
    }


    const DataTypeList& Context::get_types() const noexcept
    {
        return data_types;
    }


    DataTypeList&& Context::take_types() noexcept
    {
        return std::move(data_types);
    }


    // Compile a single token into byte-code instruction(s).
    void Context::compile_token(const source::Token& token)
    {
        auto [ found, word ] = (   token.get_type() != source::Token::Type::string
                                && token.get_type() != source::Token::Type::none)
                               ? runtime.find(token.get_as_word())
                               : std::tuple<bool, Word>(false, {});

        // If we found a word, check to see if it's immediate.  If it is, execute it now.
        if (found)
        {
            if (word.get_context() == WordExecutionContext::compile_time)
            {
                runtime.execute(word);
            }
            else
            {
                // At this phase we always use the word name for the execute instruction.  It is in
                // the second phase where we resolve the name to it's actual handler.
                insert_instruction(token.get_location(),
                                    Instruction::Id::execute,
                                    word.get_name());
            }
        }
        else
        {
            switch (token.get_type())
            {
                case source::Token::Type::string:
                    insert_instruction(token.get_location(),
                                       Instruction::Id::push_constant_value,
                                       token.get_text());
                    break;

                case source::Token::Type::integer:
                    insert_instruction(token.get_location(),
                                       Instruction::Id::push_constant_value,
                                       token.get_integer());
                    break;

                case source::Token::Type::floating:
                    insert_instruction(token.get_location(),
                                       Instruction::Id::push_constant_value,
                                       token.get_number());
                    break;

                case source::Token::Type::none:
                    throw_error(runtime, "Attempted to compile a None token.");
                    break;

                case source::Token::Type::word:
                    insert_instruction(token.get_location(),
                                       Instruction::Id::execute,
                                       token.get_as_word());
                    break;

                default:
                    throw_error(runtime, "Unknown token type.");
            }
        }
    }


    // Compile tokens until a given word token is found.
    std::string Context::compile_until_words(const std::vector<std::string>& words)
    {
        // Is the word name one of the words we are looking for?  If it is, return a true and the
        // name of the word we found.
        auto is_one_of_words = [&](const source::Token& match) -> std::tuple<bool, std::string>
            {
                // Make sure that the given token is allowed to be a word name.
                if (match.get_type() != source::Token::Type::string)
                {
                    // Ok, it's a valid word name, so convert the token to a string.
                    auto match_string = match.get_as_word();

                    // Check to see if we have a match with our word list...
                    for (const auto& word : words)
                    {
                        if (word == match_string)
                        {
                            // We found a match, so return it.
                            return { true, word };
                        }
                    }
                }

                // We didn't find a match, so return false and an empty string.
                return { false, "" };
            };

        // Keep track of the location of the first token in the list, so we can throw an error
        // with the location of the start of the search if we don't find a match.  That should
        // be more useful than the location of the last token we checked.
        std::optional<source::Location> start_location;

        // Loop through the tokens in the list until we find a match or the end of the list is
        // reached.
        while (current_token < (tokens.size() - 1))
        {
            // Grab the next token in the list.  If we haven't recorded a starting location, do
            // so now.
            const auto& token = get_next_token();

            if (!start_location)
            {
                start_location = token.get_location();
            }

            // Is this one of the word token's we're looking for?
            auto [ found, word ] = is_one_of_words(token);

            if (found)
            {
                // It is, so we're done here.
                return word;
            }

            // We haven't found our word, so compile the token as normal.
            compile_token(token);
        }

        // If we got here, then we didn't find a match, so throw an error.
        std::string message;

        // Generate a message to let the user know what word we were looking for and where the
        // search started so they can find it in their source.
        if (words.size() == 1)
        {
            message = "Missing word " + words[0] + " in source.";
        }
        else
        {
            std::stringstream stream;

            stream << "Missing matching word, expected one of [ ";

            for (auto word : words)
            {
                stream << word << " ";
            }

            stream << "]";

            message = stream.str();
        }

        // Throw the message to the caller.  Again, we don't use the interpreter's location
        // because it's not as useful as the start location of the search.
        //
        // TODO: Include the call stack as well?
        throw_error(start_location.value(), message);
    }


    // Get the next token in the list of tokens.
    const source::Token& Context::get_next_token()
    {
        throw_error_if(current_token >= tokens.size(),
                       runtime,
                       "Attempted to read past end of token stream.");

        ++current_token;
        return tokens[current_token];
    }


    void Context::append_script(const byte_code::ScriptPtr& script)
    {
        sub_scripts.push_back(script);
    }


    // Create a new construction an push it onto the stack.
    void Context::new_construction() noexcept
    {
        constructions.push(Construction());
    }


    // Create a new construction with the given name and location and push it onto the stack.
    void Context::new_construction(const source::Location& location,
                                   const std::string& name) noexcept
    {
        constructions.push(Construction(location, name));
    }


    // Create a new unnamed construction with the given byte-code and push it onto the stack.
    void Context::new_construction(ByteCode&& code) noexcept
    {
        constructions.push(Construction(std::move(code)));
    }


    // Pop the top construction off the stack and return it.
    Construction Context::drop_construction()
    {
        throw_error_if(constructions.empty(), runtime, "No construction to drop.");

        auto top = std::move(constructions.top());
        constructions.pop();

        return top;
    }


    // Merge the top construction on the stack into the one below it.
    void Context::merge_constructions()
    {
        throw_error_if(constructions.size() < 2, runtime, "No construction to merge into.");

        auto other = std::move(drop_construction());
        auto& code = get_construction().get_code();

        code.insert(code.end(), other.get_code().begin(), other.get_code().end());
    }


    // Get the top construction on the stack.
    Construction& Context::get_construction()
    {
        throw_error_if(constructions.empty(), runtime, "No construction on the stack to access.");

        return constructions.top();
    }


    // Set where in the top construction's code the next instructions should be inserted.
    void Context::set_insertion_point(CodeInsertionPoint point) noexcept
    {
        insertion_point = point;
    }


    // Insert an instruction into the top construction's code at the current insertion
    // point.
    void Context::insert_instruction(const Instruction& instruction)
    {
        if (insertion_point == CodeInsertionPoint::end)
        {
            get_construction().get_code().push_back(instruction);
        }
        else
        {
            get_construction().get_code().insert(get_construction().get_code().begin(),
                                                 instruction);
        }
    }


    // Insert an instruction into the top construction's code at the current insertion
    // point.
    void Context::insert_instruction(const source::Location& location,
                                     Instruction::Id id,
                                     const run_time::Value& value)
    {
        insert_instruction(Instruction(location, id, value));
    }


    // Insert an instruction into the top construction's code at the current insertion
    // point.
    void Context::insert_instruction(Instruction::Id id, const run_time::Value& value)
    {
        insert_instruction(Instruction(id, value));
    }


    // Insert an instruction into the top construction's code at the current insertion
    // point.
    void Context::insert_instruction(Instruction::Id id)
    {
        insert_instruction(Instruction(id));
    }


}

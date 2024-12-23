
#include "sorthc.h"



namespace sorthc::compilation::run_time::built_in_words
{


    namespace
    {


        void word_op_def_variable(CompilerRuntime& runtime)
        {
            auto value = runtime.pop_as_string();

            runtime.get_compile_context()
                   .insert_instruction(byte_code::Instruction::Id::def_variable, value);
        }


        void word_op_def_constant(CompilerRuntime& runtime)
        {
            auto value = runtime.pop_as_string();

            runtime.get_compile_context()
                   .insert_instruction(byte_code::Instruction::Id::def_constant, value);
        }


        void word_op_read_variable(CompilerRuntime& runtime)
        {
            runtime.get_compile_context()
                   .insert_instruction(byte_code::Instruction::Id::read_variable);
        }


        void word_op_write_variable(CompilerRuntime& runtime)
        {
            runtime.get_compile_context()
                   .insert_instruction(byte_code::Instruction::Id::write_variable);
        }


        void word_op_execute(CompilerRuntime& runtime)
        {
            auto value = runtime.pop_as_string();

            runtime.get_compile_context()
                   .insert_instruction(byte_code::Instruction::Id::execute, value);
        }


        void word_op_push_constant_value(CompilerRuntime& runtime)
        {
            runtime.get_compile_context()
                   .insert_instruction(byte_code::Instruction::Id::push_constant_value,
                                       runtime.pop());
        }


        void word_mark_loop_exit(CompilerRuntime& runtime)
        {
            runtime.get_compile_context()
                   .insert_instruction(byte_code::Instruction::Id::mark_loop_exit, runtime.pop());
        }


        void word_unmark_loop_exit(CompilerRuntime& runtime)
        {
            runtime.get_compile_context()
                   .insert_instruction(byte_code::Instruction::Id::unmark_loop_exit);
        }


        void word_op_mark_catch(CompilerRuntime& runtime)
        {
            runtime.get_compile_context()
                   .insert_instruction(byte_code::Instruction::Id::mark_catch, runtime.pop());
        }


        void word_op_unmark_catch(CompilerRuntime& runtime)
        {
            runtime.get_compile_context()
                   .insert_instruction(byte_code::Instruction::Id::unmark_catch);
        }


        void word_op_mark_context(CompilerRuntime& runtime)
        {
            runtime.get_compile_context()
                   .insert_instruction(byte_code::Instruction::Id::mark_context);
        }


        void word_op_release_context(CompilerRuntime& runtime)
        {
            runtime.get_compile_context()
                   .insert_instruction(byte_code::Instruction::Id::release_context);
        }


        void word_op_jump(CompilerRuntime& runtime)
        {
            runtime.get_compile_context()
                   .insert_instruction(byte_code::Instruction::Id::jump, runtime.pop());
        }


        void word_op_jump_if_zero(CompilerRuntime& runtime)
        {
            runtime.get_compile_context()
                   .insert_instruction(byte_code::Instruction::Id::jump_if_zero, runtime.pop());
        }


        void word_op_jump_if_not_zero(CompilerRuntime& runtime)
        {
            runtime.get_compile_context()
                   .insert_instruction(byte_code::Instruction::Id::jump_if_not_zero, runtime.pop());
        }


        void word_jump_loop_start(CompilerRuntime& runtime)
        {
            runtime.get_compile_context()
                   .insert_instruction(byte_code::Instruction::Id::jump_loop_start);
        }


        void word_jump_loop_exit(CompilerRuntime& runtime)
        {
            runtime.get_compile_context()
                   .insert_instruction(byte_code::Instruction::Id::jump_loop_exit);
        }


        void word_op_jump_target(CompilerRuntime& runtime)
        {
            runtime.get_compile_context()
                   .insert_instruction(byte_code::Instruction::Id::jump_target, runtime.pop());
        }


        void word_code_new_block(CompilerRuntime& runtime)
        {
            runtime.get_compile_context().new_construction();
        }


        void word_code_drop_stack_block(CompilerRuntime& runtime)
        {
            runtime.get_compile_context().drop_construction();
        }


        void word_code_merge_stack_block(CompilerRuntime& runtime)
        {
            runtime.get_compile_context().merge_constructions();
        }


        void word_code_pop_stack_block(CompilerRuntime& runtime)
        {
            runtime.push(runtime.get_compile_context().drop_construction().get_code());
        }


        void word_code_push_stack_block(CompilerRuntime& runtime)
        {
            auto top = runtime.pop_as_byte_code();

            runtime.get_compile_context().new_construction(std::move(top));
        }


        void word_code_stack_block_size(CompilerRuntime& runtime)
        {
            size_t size = runtime.get_compile_context().get_construction().get_code().size();

            runtime.push(static_cast<int64_t>(size));
        }


        void word_code_resolve_jumps(CompilerRuntime& runtime)
        {
            auto is_jump = [](const byte_code::Instruction& code) -> bool
                {
                    return    (code.get_id() == byte_code::Instruction::Id::jump)
                           || (code.get_id() == byte_code::Instruction::Id::jump_if_not_zero)
                           || (code.get_id() == byte_code::Instruction::Id::jump_if_zero)
                           || (code.get_id() == byte_code::Instruction::Id::mark_loop_exit)
                           || (code.get_id() == byte_code::Instruction::Id::mark_catch);
                };

            auto& top_code = runtime.get_compile_context().get_construction().get_code();

            std::list<size_t> jump_indicies;
            std::unordered_map<std::string, size_t> jump_targets;

            for (size_t i = 0; i < top_code.size(); ++i)
            {
                if (   is_jump(top_code[i])
                    && (top_code[i].get_value().is_string()))
                {
                    jump_indicies.push_back(i);
                }
                else if (   (top_code[i].get_id() == byte_code::Instruction::Id::jump_target)
                        && (top_code[i].get_value().is_string()))
                {
                    jump_targets.insert({ top_code[i].get_value().get_string(runtime), i });
                    top_code[i].get_value() = (int64_t)0;
                }
            }

            for (auto jump_index : jump_indicies)
            {
                auto& jump_op = top_code[jump_index];

                auto jump_name = jump_op.get_value().get_string(runtime);
                auto iter = jump_targets.find(jump_name);

                if (iter != jump_targets.end())
                {
                    auto target_index = iter->second;
                    jump_op.get_value() = (int64_t)target_index - (int64_t)jump_index;
                }
            }
        }


        void word_code_compile_until_words(CompilerRuntime& runtime)
        {
            auto count = runtime.pop_as_integer();
            std::vector<std::string> word_list;

            for (int64_t i = 0; i < count; ++i)
            {
                word_list.push_back(runtime.pop_as_string());
            }

            auto found = runtime.get_compile_context().compile_until_words(word_list);

            runtime.push(found);
        }


        void word_code_insert_at_front(CompilerRuntime& runtime)
        {
            runtime.get_compile_context()
                   .set_insertion_point(runtime.pop_as_bool()
                                        ? byte_code::CodeInsertionPoint::beginning
                                        : byte_code::CodeInsertionPoint::end);
        }


        void word_word(CompilerRuntime& runtime)
        {
            runtime.push(runtime.get_compile_context().get_next_token());
        }


        void word_word_index(CompilerRuntime& runtime)
        {
            auto name = runtime.get_compile_context().get_next_token().get_as_word();
            auto [ found, word ] = runtime.find(name);

            if (found)
            {
                runtime.get_compile_context()
                       .insert_instruction(byte_code::Instruction::Id::push_constant_value,
                                           (int64_t)word.get_handler_index());
            }
            else
            {
                runtime.get_compile_context()
                       .insert_instruction(byte_code::Instruction::Id::word_index, name);
            }
        }


        void word_execute(CompilerRuntime& runtime)
        {
            auto word_value = runtime.pop();

            if (word_value.is_numeric())
            {
                auto index = static_cast<size_t>(word_value.get_int(runtime));

                runtime.execute(index);
            }
            else if (word_value.is_string())
            {
                auto name = word_value.get_string(runtime);

                runtime.execute(name);
            }
            else
            {
                throw_error(runtime, "Unexpected value type for execute.");
            }
        }


        void word_is_defined(CompilerRuntime& runtime)
        {
            const auto& name = runtime.get_compile_context().get_next_token().get_as_word();

            runtime.get_compile_context()
                   .insert_instruction(byte_code::Instruction::Id::word_exists, name);
        }


        void word_is_defined_im(CompilerRuntime& runtime)
        {
            const auto& name = runtime.get_compile_context().get_next_token().get_as_word();
            auto found = std::get<0>(runtime.find(name));

            runtime.push(found);
        }


        void word_is_undefined_im(CompilerRuntime& runtime)
        {
            const auto& name = runtime.get_compile_context().get_next_token().get_as_word();
            auto found = std::get<0>(runtime.find(name));

            runtime.push(!found);
        }


        void word_dup(CompilerRuntime& runtime)
        {
            auto next = runtime.pop();

            runtime.push(next);
            runtime.push(next);
        }


        void word_drop(CompilerRuntime& runtime)
        {
            runtime.pop();
        }


        void word_swap(CompilerRuntime& runtime)
        {
            auto a = runtime.pop();
            auto b = runtime.pop();

            runtime.push(a);
            runtime.push(b);
        }


        void word_over(CompilerRuntime& runtime)
        {
            auto a = runtime.pop();
            auto b = runtime.pop();

            runtime.push(a);
            runtime.push(b);
            runtime.push(a);
        }


        void word_rot(CompilerRuntime& runtime)
        {
            auto c = runtime.pop();
            auto b = runtime.pop();
            auto a = runtime.pop();

            runtime.push(c);
            runtime.push(a);
            runtime.push(b);
        }


        void word_start_word(CompilerRuntime& runtime)
        {
            // Get the name and location of the word we are defining from the next token.
            auto& context = runtime.get_compile_context();
            const auto& next_token = context.get_next_token();

            // Make sure that the next token is valid for a word name.
            if (   (next_token.get_type() != source::Token::Type::word)
                && (next_token.get_type() != source::Token::Type::integer)
                && (next_token.get_type() != source::Token::Type::floating))
            {
                throw_error(runtime, "Expected a word name.");
            }

            // Create a new construction for the word.
            context.new_construction(next_token.get_location(),
                                     next_token.get_as_word());
        }


        void word_end_word(CompilerRuntime& runtime)
        {
            // Get the current construction off of the stack.
            auto construction = runtime.get_compile_context().drop_construction();

            // When should this word be executed?
            if (construction.get_execution_context() == WordExecutionContext::compile_time)
            {
                // It's an immediate word, so compile it now and add it to the interpreter's
                // run-time for running at compile time.
                compilation::byte_code::get_jit_engine().jit_compile(runtime, construction);
            }
            else
            {
                // It's a run-time word, so add the word to the word cache, for later compilation
                // into the final executable.
                runtime.get_compile_context().add_script_word(construction);
            }
        }


        void word_immediate(CompilerRuntime& runtime)
        {
            runtime.get_compile_context()
                   .get_construction()
                   .set_execution_context(WordExecutionContext::compile_time);
        }


        void word_hidden(CompilerRuntime& runtime)
        {
            runtime.get_compile_context()
                   .get_construction()
                   .set_visibility(WordVisibility::hidden);
        }


        void word_contextless(CompilerRuntime& runtime)
        {
            runtime.get_compile_context()
                   .get_construction()
                   .set_context_management(WordContextManagement::unmanaged);
        }


        void word_description(CompilerRuntime& runtime)
        {
            auto& token = runtime.get_compile_context().get_next_token();

            throw_error_if(token.get_type() != source::Token::Type::string,
                           runtime,
                           "Expected the description to be a string.");

            runtime.get_compile_context()
                   .get_construction()
                   .set_description(token.get_text());
        }


        void word_signature(CompilerRuntime& runtime)
        {
            auto& token = runtime.get_compile_context().get_next_token();

            throw_error_if(token.get_type() != source::Token::Type::string,
                           runtime,
                           "Expected the signature to be a string.");

            runtime.get_compile_context()
                   .get_construction()
                   .set_signature(token.get_text());
        }


        void word_include(CompilerRuntime& runtime)
        {
            auto path = runtime.pop_as_string();
            runtime.compile_script(path);
        }


        void word_include_im(CompilerRuntime& runtime)
        {
            auto& token = runtime.get_compile_context().get_next_token();
            runtime.compile_script(token.get_as_word());
        }


        void word_if_im(CompilerRuntime& runtime)
        {
            auto is_one_of = [](source::Token& match, std::vector<std::string>& words) -> bool
                {
                    if (match.get_type() != source::Token::Type::word)
                    {
                        return false;
                    }

                    auto match_text = match.get_text();

                    for (auto& word : words)
                    {
                        if (match_text == word)
                        {
                            return true;
                        }
                    }

                    return false;
                };

            auto skip_until = [&](std::vector<std::string> words) -> std::string
                {
                    auto token = runtime.get_compile_context().get_next_token();

                    while (!is_one_of(token, words))
                    {
                        token = runtime.get_compile_context().get_next_token();
                    }

                    return token.get_text();
                };


            auto result = runtime.pop_as_bool();

            if (result)
            {
                auto found = runtime.get_compile_context().compile_until_words(
                    {
                        "[else]",
                        "[then]"
                    });

                if (found == "[else]")
                {
                    skip_until({ "[then]" });
                }
            }
            else
            {
                auto found = skip_until({ "[else]", "[then]" });

                if (found == "[else]")
                {
                    runtime.get_compile_context().compile_until_words({ "[then]" });
                }
            }
        }


        void word_throw(CompilerRuntime& runtime)
        {
            throw_error(runtime, runtime.pop_as_string());
        }


        void word_equal(CompilerRuntime& runtime)
        {
            auto b = runtime.pop();
            auto a = runtime.pop();

            runtime.push(a == b);
        }


        void word_greater_equal(CompilerRuntime& runtime)
        {
            auto b = runtime.pop();
            auto a = runtime.pop();

            runtime.push((a <=> b == std::strong_ordering::greater) || a == b);
        }


        void word_less_equal(CompilerRuntime& runtime)
        {
            auto b = runtime.pop();
            auto a = runtime.pop();

            runtime.push((a <=> b == std::strong_ordering::less) || a == b);
        }


        void word_greater(CompilerRuntime& runtime)
        {
            auto b = runtime.pop();
            auto a = runtime.pop();

            runtime.push(a <=> b == std::strong_ordering::greater);
        }


        void word_less(CompilerRuntime& runtime)
        {
            auto b = runtime.pop();
            auto a = runtime.pop();

            runtime.push(a <=> b == std::strong_ordering::less);
        }


        void word_none(CompilerRuntime& runtime)
        {
            runtime.push(Value());
        }


        void word_true(CompilerRuntime& runtime)
        {
            runtime.push(true);
        }


        void word_false(CompilerRuntime& runtime)
        {
            runtime.push(false);
        }


        void word_unique_str(CompilerRuntime& runtime)
        {
            static std::atomic<int64_t> index = 0;

            std::stringstream stream;
            auto current = index.fetch_add(1, std::memory_order_relaxed);

            stream << "unique-" << std::setw(4) << std::setfill('0') << std::hex << current;

            runtime.push(stream.str());
        }


    }


    // Register the built-in words with the compiler run-time.
    void register_words(CompilerRuntime& runtime)
    {
        // Byte-code creation and manipulation words.
        ADD_NATIVE_WORD(runtime, "op.def_variable", word_op_def_variable);
        ADD_NATIVE_WORD(runtime, "op.def_constant", word_op_def_constant);
        ADD_NATIVE_WORD(runtime, "op.read_variable", word_op_read_variable);
        ADD_NATIVE_WORD(runtime, "op.write_variable", word_op_write_variable);
        ADD_NATIVE_WORD(runtime, "op.execute", word_op_execute);
        ADD_NATIVE_WORD(runtime, "op.push_constant_value", word_op_push_constant_value);
        ADD_NATIVE_WORD(runtime, "op.mark_loop_exit", word_mark_loop_exit);
        ADD_NATIVE_WORD(runtime, "op.unmark_loop_exit", word_unmark_loop_exit);
        ADD_NATIVE_WORD(runtime, "op.mark_catch", word_op_mark_catch);
        ADD_NATIVE_WORD(runtime, "op.unmark_catch", word_op_unmark_catch);
        ADD_NATIVE_WORD(runtime, "op.mark_context", word_op_mark_context);
        ADD_NATIVE_WORD(runtime, "op.release_context", word_op_release_context);
        ADD_NATIVE_WORD(runtime, "op.jump", word_op_jump);
        ADD_NATIVE_WORD(runtime, "op.jump_if_zero", word_op_jump_if_zero);
        ADD_NATIVE_WORD(runtime, "op.jump_if_not_zero", word_op_jump_if_not_zero);
        ADD_NATIVE_WORD(runtime, "op.jump_loop_start", word_jump_loop_start);
        ADD_NATIVE_WORD(runtime, "op.jump_loop_exit", word_jump_loop_exit);
        ADD_NATIVE_WORD(runtime, "op.jump_target", word_op_jump_target);
        ADD_NATIVE_WORD(runtime, "code.new_block", word_code_new_block);
        ADD_NATIVE_WORD(runtime, "code.drop_stack_block", word_code_drop_stack_block);
        ADD_NATIVE_WORD(runtime, "code.merge_stack_block", word_code_merge_stack_block);
        ADD_NATIVE_WORD(runtime, "code.pop_stack_block", word_code_pop_stack_block);
        ADD_NATIVE_WORD(runtime, "code.push_stack_block", word_code_push_stack_block);
        ADD_NATIVE_WORD(runtime, "code.stack-block-size@", word_code_stack_block_size);
        ADD_NATIVE_WORD(runtime, "code.resolve_jumps", word_code_resolve_jumps);
        ADD_NATIVE_WORD(runtime, "code.compile_until_words", word_code_compile_until_words);
        ADD_NATIVE_WORD(runtime, "code.insert_at_front", word_code_insert_at_front);

        // Word words.
        ADD_NATIVE_WORD(runtime, "word", word_word);
        ADD_NATIVE_IMMEDIATE_WORD(runtime, "`", word_word_index);
        ADD_NATIVE_WORD(runtime, "execute", word_execute);
        ADD_NATIVE_IMMEDIATE_WORD(runtime, "defined?", word_is_defined);
        ADD_NATIVE_IMMEDIATE_WORD(runtime, "[defined?]", word_is_defined_im);
        ADD_NATIVE_IMMEDIATE_WORD(runtime, "[undefined?]", word_is_undefined_im);

        ADD_NATIVE_WORD(runtime, "dup", word_dup);
        ADD_NATIVE_WORD(runtime, "drop", word_drop);
        ADD_NATIVE_WORD(runtime, "swap", word_swap);
        ADD_NATIVE_WORD(runtime, "over", word_over);
        ADD_NATIVE_WORD(runtime, "rot", word_rot);

        // Word creation words.
        ADD_NATIVE_IMMEDIATE_WORD(runtime, ":", word_start_word);
        ADD_NATIVE_IMMEDIATE_WORD(runtime, ";", word_end_word);
        ADD_NATIVE_IMMEDIATE_WORD(runtime, "immediate", word_immediate);
        ADD_NATIVE_IMMEDIATE_WORD(runtime, "hidden", word_hidden);
        ADD_NATIVE_IMMEDIATE_WORD(runtime, "contextless", word_contextless);
        ADD_NATIVE_IMMEDIATE_WORD(runtime, "description:", word_description);
        ADD_NATIVE_IMMEDIATE_WORD(runtime, "signature:", word_signature);

        // Run-time state words.
        ADD_NATIVE_WORD(runtime, "include", word_include);
        ADD_NATIVE_IMMEDIATE_WORD(runtime, "[include]", word_include_im);
        ADD_NATIVE_IMMEDIATE_WORD(runtime, "[if]", word_if_im);
        ADD_NATIVE_WORD(runtime, "throw", word_throw);

        // Equality words.
        ADD_NATIVE_WORD(runtime, "=", word_equal);
        ADD_NATIVE_WORD(runtime, ">=", word_greater_equal);
        ADD_NATIVE_WORD(runtime, "<=", word_less_equal);
        ADD_NATIVE_WORD(runtime, ">", word_greater);
        ADD_NATIVE_WORD(runtime, "<", word_less);

        // Special value words.
        ADD_NATIVE_WORD(runtime, "none", word_none);
        ADD_NATIVE_WORD(runtime, "true", word_true);
        ADD_NATIVE_WORD(runtime, "false", word_false);
        ADD_NATIVE_WORD(runtime, "unique_str", word_unique_str);
    }


}


#include "sorthc.h"



namespace sorth::compilation
{


    namespace
    {


        // LLVM IR has rules for what characters are allowed in a symbol name.  This function will
        // filter out any characters that are not allowed.
        //
        // Additionally, we'll add unique naming and an index to ensure that duplicate names don't
        // interfere with each other.  It's valid to have multiple words with the same name in
        // Forth.
        std::string filter_word_name(const std::string& name)
        {
            static std::atomic<int64_t> index = 0;

            std::stringstream stream;
            auto current = index.fetch_add(1, std::memory_order_relaxed);

            stream << "_word_fn_";

            for (auto c : name)
            {
                switch (c)
                {
                    case '@':   stream << "_at_";       break;
                    case '\'':  stream << "_prime_";    break;
                    case '"':   stream << "_quote_";    break;
                    case '%':   stream << "_percent_";  break;
                    case '!':   stream << "_bang_";     break;
                    case '?':   stream << "_question_"; break;

                    default:    stream << c;            break;
                }
            }

            stream << "_index_"
                   << std::setw(6) << std::setfill('0') << current
                   << "_";

            return stream.str();
        }


        // Map of word names to their indicies in the word list.
        using WordMap = std::unordered_map<std::string, size_t>;


        // Information about a word that the compiler knows about.
        struct WordInfo
        {
            std::string name;           // The Forth name of the word.
            std::string handler_name;   // The name of the function that implements the word.
            bool was_referenced;        // True if the word was referenced by the script.

            std::optional<byte_code::ByteCode> code;  // If unassigned, then it's a native word.

            llvm::Function* function;   // The compiled function or declaration for the word.
        };


        using WordInfoList = std::vector<WordInfo>;


        struct WordCollection;
        void try_resolve_calls(WordCollection& collection, byte_code::ByteCode& code);


        // Collection of words that the compiler knows about.  They can come from the runtime, the
        // standard library, or the program being compiled.
        struct WordCollection
        {
            WordInfoList words;
            WordMap word_map;

            // Add a native word to our collection.
            void add_word(const std::string& name, const std::string& handler_name)
            {
                WordInfo info;

                info.name = name;
                info.handler_name = handler_name;
                info.was_referenced = false;
                // Code is not assigned because it's a native word.
                info.function = nullptr;

                words.push_back(info);
                word_map[name] = words.size() - 1;
            }

            // Add a Forth word to our collection.
            void add_word(const byte_code::Construction& word)
            {
                // Cache the word's name.
                const auto& name = word.get_name();

                // Copy over the word's information that we care about at this level.
                WordInfo info;

                info.name = name;
                info.handler_name = filter_word_name(name);
                info.code = word.get_code();
                info.was_referenced = false;
                info.function = nullptr;

                // Try to resolve any calls in the word's code.  We know for sure that some calls
                // will remain unresolved, for example calls to constant and variable index words.
                try_resolve_calls(*this, info.code.value());

                // Add to the collections.
                words.push_back(info);
                word_map[name] = words.size() - 1;
            }
        };


        // Try to resolve calls to words in the word's code.  This way if a word is overwritten at a
        // later point, this word will still call the correct word.
        void try_resolve_calls(WordCollection& collection, byte_code::ByteCode& code)
        {
            for (size_t i = 0; i < code.size(); ++i)
            {
                // Get the current instruction.
                auto& instruction = code[i];

                // First make sure we're dealing with an execute instruction.
                if (instruction.get_id() == byte_code::Instruction::Id::execute)
                {
                    // If the value is still unresolved...
                    if (instruction.get_value().is_string())
                    {
                        // Try to resolve the word name to it's index.
                        auto& name = instruction.get_value().get_string();
                        auto iterator = collection.word_map.find(name);

                        if (iterator != collection.word_map.end())
                        {
                            // We found the word, so update the instruction to call the word by it's
                            // index.  We don't mark this word as referenced yet, because we don't
                            // know if the word we're resolving is actually referenced.
                            instruction.get_value() = iterator->second;
                        }
                    }
                }
            }
        }


        // Try to resolve all the outstanding calls in the collection of words.
        void try_resolve_words(WordCollection& collection)
        {
            // Go through the collection of words...
            for (auto& word : collection.words)
            {
                // If the word isn't a native one...
                if (word.code.has_value())
                {
                    // Try to resolve all the outstanding calls in the word's code.
                    try_resolve_calls(collection, word.code.value());
                }
            }
        }


        void gather_runtime_words(WordCollection& collection)
        {
            auto collector = [&collection](const std::string& name, const std::string& handler_name)
                {
                    collection.add_word(name, handler_name);
                };

            sorth::run_time::abi::words::register_runtime_words(collector);
        }


        // Gather all the words in the script and it's subscripts.
        void gather_script_words(const byte_code::ScriptPtr& script, WordCollection& collection)
        {
            //
        }


        void collect_top_level_code(const byte_code::ScriptPtr& script)
        {
            //
        }


        void mark_used_words(WordCollection& collection, const byte_code::ByteCode& top_level_code)
        {
            //
        }


        // Create the function declaration for a word found in the collection.
        void create_word_declaration(WordInfo& word,
                                     std::shared_ptr<llvm::Module>& module,
                                     llvm::LLVMContext& context)
        {
            // Create the signature for the word's function: int8_t word_fn(void);
            llvm::FunctionType* signature = llvm::FunctionType::get(llvm::Type::getInt8Ty(context),
                                                                    false);

            // If the word is a native word, then it's external and will be linked in later.
            // Otherwise the word is a Forth word and will be internal to the module.  Be it a
            // standard library word or one from the user script(s).
            auto linkage = word.code.has_value() ? llvm::Function::InternalLinkage
                                                 : llvm::Function::ExternalLinkage;

            // Create the function signature itself.  If it's a Forth word, it's body will be filled
            // out later.
            word.function = llvm::Function::Create(signature,
                                                   linkage,
                                                   word.handler_name,
                                                   module.get());
        }


        // Create the function definitions for all of the words in the collection, both native and
        // Forth words.  But only if it was referenced by a script top-level.
        void create_word_declarations(WordCollection& collection,
                                      std::shared_ptr<llvm::Module>& module,
                                      llvm::LLVMContext& context)
        {
            for (auto& word : collection.words)
            {
                if (word.was_referenced)
                {
                    create_word_declaration(word, module, context);
                }
            }
        }


        // Compile the llvm IR for a word.
        void compile_word(const WordCollection& collection,
                          const WordInfo& word,
                          const std::shared_ptr<llvm::Module>& module,
                          llvm::IRBuilder<>& builder)
        {
            //
        }


        // Go through the collection of words and compile the ones that were referenced.
        void compile_used_words(WordCollection& collection,
                                const std::shared_ptr<llvm::Module>& module,
                                llvm::LLVMContext& context,
                                llvm::IRBuilder<>& builder)
        {
            for (const auto& word : collection.words)
            {
                // If the word was referenced and is a Forth word...
                if (   (word.was_referenced)
                    && (word.code.has_value()))
                {
                    // Create the word's IR function body.
                    compile_word(collection, word, module, builder);
                }
            }
        }


        void compile_top_level_code(const byte_code::ByteCode& top_level_code,
                                    const std::shared_ptr<llvm::Module>& module,
                                    llvm::LLVMContext& context,
                                    llvm::IRBuilder<>& builder)
        {
            // Start off by creating the signature for the top-level function.
            llvm::FunctionType* top_level_signature =
                                            llvm::FunctionType::get(llvm::Type::getInt8Ty(context),
                                                                    false);

            // Create the function itself.
            llvm::Function* top_level_function =
                                             llvm::Function::Create(top_level_signature,
                                                                    llvm::Function::ExternalLinkage,
                                                                    "script_top_level",
                                                                    module.get());

            auto entry_block = llvm::BasicBlock::Create(context, "entry", top_level_function);

            builder.SetInsertPoint(entry_block);

            // Generate the return statement for the top-level function.
            auto ret_val = llvm::ConstantInt::get(builder.getInt8Ty(), EXIT_SUCCESS);
            builder.CreateRet(ret_val);
        }


        void optimize_module(const std::shared_ptr<llvm::Module>& module)
        {
            // Create a pass manager for the module.
            /*llvm::legacy::PassManager pass_manager;

            // Add the target data to the pass manager.
            module->setDataLayout(llvm::sys::getDefaultDataLayout());
            pass_manager.add(llvm::createTargetTransformInfoWrapperPass(module->getDataLayout()));

            // Add the optimization passes to the pass manager.
            llvm::PassManagerBuilder pass_manager_builder;
            pass_manager_builder.OptLevel = 3;
            pass_manager_builder.SizeLevel = 0;
            pass_manager_builder.Inliner = llvm::createFunctionInliningPass();
            pass_manager_builder.LoopVectorize = true;
            pass_manager_builder.SLPVectorize = true;

            pass_manager_builder.populateModulePassManager(pass_manager);

            // Run the optimization passes on the module.
            pass_manager.run(*module);*/
        }


    }


    // Generate the LLVM IR for a script and it's sub-scripts, and write the resulting IR to an
    // object file.
    void generate_llvm_ir(const byte_code::ScriptPtr& standard_library,
                          const byte_code::ScriptPtr& script,
                          const std::filesystem::path& output_path)
    {
        // Create the LLVM context for the compilation, then create the module that will hold the
        // generated LLVM IR.
        llvm::LLVMContext context;

        auto module = std::make_shared<llvm::Module>(script->get_script_path().string(), context);
        llvm::IRBuilder<> builder(context);

        // Gather all the words from the runtime, the standard library, and the script and it's
        // sub-scripts.
        WordCollection words;

        gather_runtime_words(words);
        gather_script_words(standard_library, words);
        gather_script_words(script, words);

        // Now that all words have been gathered, we can try to resolve all calls one last time.
        // After this the only unresolved calls should be variable and constant accesses which will
        // get resolved later.  Any non-variable or constant calls that are unresolved at this point
        // are a fatal error, which we'll check for during the code generation phase.
        try_resolve_words(words);

        // Build up the full top-level code for the script, including any sub-scripts and the
        // standard library.
        byte_code::ByteCode top_level_code;

        collect_top_level_code(standard_library);
        collect_top_level_code(script);

        // Now that we have all the top-levels collected, we can go through that code and mark
        // words as used or unused.  Then make sure that all of the used words have been properly
        // declared.
        mark_used_words(words, top_level_code);
        create_word_declarations(words, module, context);

        // Compile the function bodies for all used Forth words.
        compile_used_words(words, module, context, builder);

        // Create the script top-level function.  This function will be the entry point for the
        // resulting program.  It'll be made of of all the top level blocks in each of the scripts
        // that we gathered previously.
        compile_top_level_code(top_level_code, module, context, builder);

        // Now that all code has been generated, verify that the module is well-formed.
        if (verifyModule(*module, &llvm::errs()))
        {
            throw_error("Generated LLVM IR module is invalid.");
        }

        // Apply LLVM optimization passes to the module.
        optimize_module(module);

        // Write the LLVM IR to an object file.

        // Get the target triple for the host machine.
        auto target_triple = llvm::sys::getDefaultTargetTriple();

        // Find the llvm target for the host machine.
        std::string error;
        const auto target = llvm::TargetRegistry::lookupTarget(target_triple, error);

        if (!target)
        {
            throw_error("Failed to lookup LLVM target: " + error);
        }

        // Set the module's target triple and data layout.
        llvm::TargetOptions options;

        auto reloc_model = std::optional<llvm::Reloc::Model>();
        auto target_machine = target->createTargetMachine(target_triple,
                                                          "generic",
                                                          "",
                                                          options,
                                                          reloc_model);

        module->setTargetTriple(target_triple);
        module->setDataLayout(target_machine->createDataLayout());

        // Print the module to stdout for debugging.
        module->print(llvm::outs(), nullptr);

        // Write the module to an object file while compiling it to native code.
        std::error_code error_code;
        llvm::raw_fd_ostream output_stream(output_path.string(), error_code);

        if (error_code)
        {
            throw_error("Failed to open output file: " + error_code.message());
        }

        llvm::legacy::PassManager pass_manager;

        if (target_machine->addPassesToEmitFile(pass_manager,
                                                output_stream,
                                                nullptr,
                                                llvm::CodeGenFileType::ObjectFile))
        {
            throw_error("Failed to setup target machine to emit object file.");
        }

        pass_manager.run(*module);
        output_stream.flush();
    }


}

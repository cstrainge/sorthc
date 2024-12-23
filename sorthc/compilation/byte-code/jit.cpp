
#include "sorthc.h"



namespace sorthc::compilation::byte_code
{


    namespace
    {


        // On macOS symbols must be prefixed by an underscore.  We'll use this constant to handle
        // that later on in the code.
        #if defined(IS_MACOS)
            const std::string platform_prefix = "_";
        #else
            const std::string platform_prefix = "";
        #endif


    }


    thread_local std::optional<std::runtime_error> Jit::last_exception;


    Jit& get_jit_engine()
    {
        static Jit jit;

        return jit;
    }


    Jit::Jit()
    {
        // Initialize the llvm libraries.
        llvm::InitializeNativeTarget();
        llvm::InitializeNativeTargetAsmPrinter();
        llvm::InitializeNativeTargetAsmParser();
        llvm::InitializeNativeTargetDisassembler();

        // Construct the LLVM JIT engine, using the object linking layer creator to create
        // and use our custom memory manager.
        auto jit_result = llvm::orc::LLJITBuilder().create();

        // Check to see if we were successful.
        if (!jit_result)
        {
            std::string error_message;

            llvm::raw_string_ostream stream(error_message);
            llvm::logAllUnhandledErrors(jit_result.takeError(),
                                        stream,
                                        "Failed to create llvm JIT engine: ");
            stream.flush();

            throw_error(error_message);
        }

        // Save the jit engine and register the helper function symbols/types.
        jit = std::move(*jit_result);
        register_jit_helper_ptrs();
    }


    // JIT compile a word construction into native code and register it for running within the
    // compiler's runtime.  This can be an immediate word or run-time word intended for use by
    // immediate words.
    void Jit::jit_compile(run_time::CompilerRuntime& runtime, const Construction& construction)
    {
        // Make sure we have a name for the word that's usable for the JIT engine.
        auto filtered_name = filter_word_name(construction.get_name());

        // Create the context for this compilation.
        auto [ module, context ] = create_jit_module_context(filtered_name);

        // Jit compile the word and then finalize it's module.
        auto [ locations, constants ] = jit_compile_word(runtime,
                                                         module,
                                                         context,
                                                         filtered_name,
                                                         construction.get_code(),
                                                         CodeGenType::word);

        // JIT compile and optimize the module, returning the IR for the word.
        auto ir_map = finalize_module(std::move(module), std::move(context));

        // Finally create and register the word handler with the runtime.
        auto handler = create_word_function(filtered_name,
                                            std::move(ir_map[filtered_name]),
                                            std::move(locations),
                                            std::move(constants),
                                            CodeGenType::word);

        runtime.add_word(construction.get_name(),
                         construction.get_location(),
                         handler,
                         construction.get_execution_context(),
                         construction.get_visibility(),
                         compilation::WordType::scripted,
                         construction.get_context_management());
    }


    // JIT compile a script into native code for running within the compiler's runtime.  Register
    // all of the scripts non-immediate words for execution for immediate words.
    //
    // The immediate words would have already been compiled and registered with the compiler's
    // runtime.
    run_time::WordHandler Jit::jit_compile(run_time::CompilerRuntime& runtime,
                                           const ScriptPtr& script)
    {
        // JIT compile and register all of the words in the script with the run-time.
        for (const auto& word_construction : script->get_words())
        {
            jit_compile(runtime, word_construction);
        }

        // Get the script's top level code.
        const auto& top_level = script->get_top_level();

        // If there is no top level code, just return a handler that does nothing.
        if (top_level.empty())
        {
            auto null_handler = [](run_time::CompilerRuntime&) -> void
                {
                    // Do nothing.
                };

            return null_handler;
        }

        // Compile the script's top-level code into a handler.  Start off by filtering the name of
        // the script for the JIT engine.
        auto filtered_name = filter_word_name(script->get_script_path().string());

        // Create the context for this compilation.
        auto [ module, context ] = create_jit_module_context(filtered_name);

        // Jit compile the word and then finalize it's module.
        auto [ locations, constants ] = jit_compile_word(runtime,
                                                         module,
                                                         context,
                                                         filtered_name,
                                                         top_level,
                                                         CodeGenType::script_body);

        // JIT compile and optimize the module, returning the IR for the word.
        auto ir_map = finalize_module(std::move(module), std::move(context));

        // Create the top level code handler for the script and return it to the caller.
        auto handler = create_word_function(filtered_name,
                                            std::move(ir_map[filtered_name]),
                                            std::move(locations),
                                            std::move(constants),
                                            CodeGenType::word);

        return handler;
    }


    // Register the helper function pointers with the JIT engine.
    void Jit::register_jit_helper_ptrs()
    {
        // Properly mangle symbols for the platform's ABI, on macOS this means we need a
        // leading underscore.
        auto mangle = [this](const std::string& name) -> llvm::orc::SymbolStringPtr
            {
                return jit->getExecutionSession().intern(platform_prefix + name);
            };

        // Create the symbol map for the helper functions.
        llvm::orc::SymbolMap symbol_map =
            {
                {
                    mangle("handle_set_location"),
                    llvm::orc::ExecutorSymbolDef(
                                            llvm::orc::ExecutorAddr((uint64_t)&handle_set_location),
                                            llvm::JITSymbolFlags::Exported)
                },
                {
                    mangle("handle_manage_context"),
                    llvm::orc::ExecutorSymbolDef(
                                          llvm::orc::ExecutorAddr((uint64_t)&handle_manage_context),
                                          llvm::JITSymbolFlags::Exported)
                },
                {
                    mangle("handle_define_variable"),
                    llvm::orc::ExecutorSymbolDef(
                                         llvm::orc::ExecutorAddr((uint64_t)&handle_define_variable),
                                         llvm::JITSymbolFlags::Exported)
                },
                {
                    mangle("handle_define_constant"),
                    llvm::orc::ExecutorSymbolDef(
                                         llvm::orc::ExecutorAddr((uint64_t)&handle_define_constant),
                                         llvm::JITSymbolFlags::Exported)
                },
                {
                    mangle("handle_read_variable"),
                    llvm::orc::ExecutorSymbolDef(
                                           llvm::orc::ExecutorAddr((uint64_t)&handle_read_variable),
                                           llvm::JITSymbolFlags::Exported)
                },
                {
                    mangle("handle_write_variable"),
                    llvm::orc::ExecutorSymbolDef(
                                          llvm::orc::ExecutorAddr((uint64_t)&handle_write_variable),
                                          llvm::JITSymbolFlags::Exported)
                },
                {
                    mangle("handle_pop_bool"),
                    llvm::orc::ExecutorSymbolDef(
                                                llvm::orc::ExecutorAddr((uint64_t)&handle_pop_bool),
                                                llvm::JITSymbolFlags::Exported)
                },
                {
                    mangle("handle_push_last_exception"),
                    llvm::orc::ExecutorSymbolDef(
                                     llvm::orc::ExecutorAddr((uint64_t)&handle_push_last_exception),
                                     llvm::JITSymbolFlags::Exported)
                },
                {
                    mangle("handle_push_bool"),
                    llvm::orc::ExecutorSymbolDef(
                                               llvm::orc::ExecutorAddr((uint64_t)&handle_push_bool),
                                               llvm::JITSymbolFlags::Exported)
                },
                {
                    mangle("handle_push_int"),
                    llvm::orc::ExecutorSymbolDef(
                                                llvm::orc::ExecutorAddr((uint64_t)&handle_push_int),
                                                llvm::JITSymbolFlags::Exported)
                },
                {
                    mangle("handle_push_double"),
                    llvm::orc::ExecutorSymbolDef(
                                             llvm::orc::ExecutorAddr((uint64_t)&handle_push_double),
                                             llvm::JITSymbolFlags::Exported)
                },
                {
                    mangle("handle_push_string"),
                    llvm::orc::ExecutorSymbolDef(
                                             llvm::orc::ExecutorAddr((uint64_t)&handle_push_string),
                                             llvm::JITSymbolFlags::Exported)
                },
                {
                    mangle("handle_push_value"),
                    llvm::orc::ExecutorSymbolDef(
                                              llvm::orc::ExecutorAddr((uint64_t)&handle_push_value),
                                              llvm::JITSymbolFlags::Exported)
                },
                {
                    mangle("handle_word_execute_name"),
                    llvm::orc::ExecutorSymbolDef(
                                       llvm::orc::ExecutorAddr((uint64_t)&handle_word_execute_name),
                                       llvm::JITSymbolFlags::Exported)
                },
                {
                    mangle("handle_word_execute_index"),
                    llvm::orc::ExecutorSymbolDef(
                                      llvm::orc::ExecutorAddr((uint64_t)&handle_word_execute_index),
                                      llvm::JITSymbolFlags::Exported)
                },
                {
                    mangle("handle_word_index_name"),
                    llvm::orc::ExecutorSymbolDef(
                                    llvm::orc::ExecutorAddr((uint64_t)&handle_word_index_name),
                                    llvm::JITSymbolFlags::Exported)
                },
                {
                    mangle("handle_word_exists_name"),
                    llvm::orc::ExecutorSymbolDef(
                                llvm::orc::ExecutorAddr((uint64_t)&handle_word_exists_name),
                                llvm::JITSymbolFlags::Exported)
                }
            };

        // Register that map with the JIT engine.
        auto error = jit->getMainJITDylib()
                                         .define(llvm::orc::absoluteSymbols(std::move(symbol_map)));

        if (error)
        {
            std::string error_message;

            llvm::raw_string_ostream stream(error_message);
            llvm::logAllUnhandledErrors(std::move(error),
                                        stream,
                                        "Failed to define helper symbols: ");
            stream.flush();

            throw_error(error_message);
        }
    }


    // Register our helper functions with the JIT module being generated.
    void Jit::register_jit_helpers(std::unique_ptr<llvm::Module>& module,
                                   std::unique_ptr<llvm::LLVMContext>& context)
    {
        // Gather up our basic types needed for the function signatures.
        auto void_type = llvm::Type::getVoidTy(*context.get());
        auto int8_type = llvm::Type::getInt8Ty(*context.get());
        auto ptr_type = llvm::PointerType::getUnqual(int8_type);
        auto int64_type = llvm::Type::getInt64Ty(*context.get());
        auto bool_type = llvm::Type::getInt1Ty(*context.get());
        auto bool_ptr_type = llvm::PointerType::getUnqual(bool_type);
        auto double_type = llvm::Type::getDoubleTy(*context.get());
        auto char_type = llvm::Type::getInt8Ty(*context.get());
        auto char_ptr_type = llvm::PointerType::getUnqual(char_type);


        // Register the handle_set_location function.
        auto handle_set_location_type = llvm::FunctionType::get(void_type,
                                                                { ptr_type, ptr_type, int64_type },
                                                                false);
        handle_set_location_fn = llvm::Function::Create(handle_set_location_type,
                                                        llvm::Function::ExternalLinkage,
                                                        "handle_set_location",
                                                        module.get());

        // Register the handle_manage_context function.
        auto handle_manage_context_type = llvm::FunctionType::get(void_type,
                                                                  { ptr_type, bool_type },
                                                                  false);
        handle_manage_context_fn = llvm::Function::Create(handle_manage_context_type,
                                                          llvm::Function::ExternalLinkage,
                                                          "handle_manage_context",
                                                          module.get());

        // Register the handle_define_variable function.
        auto handle_define_variable_type = llvm::FunctionType::get(int64_type,
                                                                   { ptr_type, char_ptr_type },
                                                                   false);
        handle_define_variable_fn = llvm::Function::Create(handle_define_variable_type,
                                                           llvm::Function::ExternalLinkage,
                                                           "handle_define_variable",
                                                           module.get());

        // Register the handle_define_constant function.
        auto handle_define_constant_type = llvm::FunctionType::get(int64_type,
                                                                   { ptr_type, char_ptr_type },
                                                                   false);
        handle_define_constant_fn = llvm::Function::Create(handle_define_constant_type,
                                                           llvm::Function::ExternalLinkage,
                                                           "handle_define_constant",
                                                           module.get());

        // Register the handle_read_variable function.
        auto handle_read_variable_type = llvm::FunctionType::get(int64_type, { ptr_type }, false);
        handle_read_variable_fn = llvm::Function::Create(handle_read_variable_type,
                                                         llvm::Function::ExternalLinkage,
                                                         "handle_read_variable",
                                                         module.get());

        // Register the handle_write_variable function.
        auto handle_write_variable_type = llvm::FunctionType::get(int64_type, { ptr_type }, false);
        handle_write_variable_fn = llvm::Function::Create(handle_write_variable_type,
                                                          llvm::Function::ExternalLinkage,
                                                          "handle_write_variable",
                                                          module.get());

        // Register the handle_pop_bool function.
        auto handle_pop_bool_type = llvm::FunctionType::get(int64_type,
                                                            { ptr_type, bool_ptr_type },
                                                            false);
        handle_pop_bool_fn = llvm::Function::Create(handle_pop_bool_type,
                                                    llvm::Function::ExternalLinkage,
                                                    "handle_pop_bool",
                                                    module.get());

        // Register the handle_push_last_exception function.
        auto handle_push_last_exception_type = llvm::FunctionType::get(void_type,
                                                                       { ptr_type },
                                                                       false);
        handle_push_last_exception_fn = llvm::Function::Create(handle_push_last_exception_type,
                                                               llvm::Function::ExternalLinkage,
                                                               "handle_push_last_exception",
                                                               module.get());

        // Register the handle_push_bool function.
        auto handle_push_bool_type = llvm::FunctionType::get(void_type,
                                                             { ptr_type, bool_type },
                                                             false);
        handle_push_bool_fn = llvm::Function::Create(handle_push_bool_type,
                                                     llvm::Function::ExternalLinkage,
                                                     "handle_push_bool",
                                                     module.get());

        // Register the handle_push_int function.
        auto handle_push_int_type = llvm::FunctionType::get(void_type,
                                                            { ptr_type, int64_type },
                                                            false);
        handle_push_int_fn = llvm::Function::Create(handle_push_int_type,
                                                    llvm::Function::ExternalLinkage,
                                                    "handle_push_int",
                                                    module.get());

        // Register the handle_push_double function.
        auto handle_push_double_type = llvm::FunctionType::get(void_type,
                                                               { ptr_type, double_type },
                                                               false);
        handle_push_double_fn = llvm::Function::Create(handle_push_double_type,
                                                       llvm::Function::ExternalLinkage,
                                                       "handle_push_double",
                                                       module.get());

        // Register the handle_push_string function.
        auto handle_push_string_type = llvm::FunctionType::get(void_type,
                                                               { ptr_type, char_ptr_type },
                                                               false);
        handle_push_string_fn = llvm::Function::Create(handle_push_string_type,
                                                       llvm::Function::ExternalLinkage,
                                                       "handle_push_string",
                                                       module.get());

        // Register the handle_push_value function.
        auto handle_push_value_type = llvm::FunctionType::get(void_type,
                                                              { ptr_type, ptr_type, int64_type },
                                                              false);
        handle_push_value_fn = llvm::Function::Create(handle_push_value_type,
                                                      llvm::Function::ExternalLinkage,
                                                      "handle_push_value",
                                                      module.get());

        // Register the handle_word_execute_name function.
        auto handle_word_execute_name_type = llvm::FunctionType::get(int64_type,
                                                                     { ptr_type, char_ptr_type },
                                                                     false);
        handle_word_execute_name_fn = llvm::Function::Create(handle_word_execute_name_type,
                                                             llvm::Function::ExternalLinkage,
                                                             "handle_word_execute_name",
                                                             module.get());

        // Register the handle_word_execute_index function.
        auto handle_word_execute_index_type = llvm::FunctionType::get(int64_type,
                                                                      { ptr_type, int64_type },
                                                                      false);
        handle_word_execute_index_fn = llvm::Function::Create(handle_word_execute_index_type,
                                                              llvm::Function::ExternalLinkage,
                                                              "handle_word_execute_index",
                                                              module.get());

        // Register the handle_word_index_name function.
        auto handle_word_index_name_type = llvm::FunctionType::get(void_type,
                                                                   { ptr_type, char_ptr_type },
                                                                   false);
        handle_word_index_name_fn = llvm::Function::Create(handle_word_index_name_type,
                                                           llvm::Function::ExternalLinkage,
                                                           "handle_word_index_name",
                                                           module.get());

        // Register the handle_word_exists_name function.
        auto handle_word_exists_name_type = llvm::FunctionType::get(void_type,
                                                                    { ptr_type, char_ptr_type },
                                                                    false);
        handle_word_exists_name_fn = llvm::Function::Create(handle_word_exists_name_type,
                                                            llvm::Function::ExternalLinkage,
                                                            "handle_word_exists_name",
                                                            module.get());
    }


    std::string Jit::filter_word_name(const std::string& name)
    {
        static std::atomic<int64_t> index = 0;

        std::stringstream stream;
        auto current = index.fetch_add(1, std::memory_order_relaxed);

        stream << "__fn_";

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

        stream << "_index_" << std::setw(6) << std::setfill('0') << current
                << "_";

        return stream.str();
    }


    std::tuple<std::unique_ptr<llvm::Module>,
                       std::unique_ptr<llvm::LLVMContext>>
                                             Jit::create_jit_module_context(const std::string& name)
    {
        // Create the llvm module that will hold the JITed code.  Then register our helper
        // functions with it.
        auto context = std::make_unique<llvm::LLVMContext>();
        auto module = std::make_unique<llvm::Module>("sorth_module_" + name, *context.get());

        register_jit_helpers(module, context);

        return { std::move(module), std::move(context) };
    }


    std::unordered_map<std::string,
                       std::string>
                                  Jit::finalize_module(std::unique_ptr<llvm::Module>&& module,
                                                       std::unique_ptr<llvm::LLVMContext>&& context)
    {
        // Capture the optimized IR for all the module's functions.
        std::unordered_map<std::string, std::string> ir_map;

        // Uncomment to print out the LLVM IR for the JITed function module but before being
        // optimized.
        // module->print(llvm::outs(), nullptr);

        // Make sure that the generated module is viable.
        std::string error_str;
        llvm::raw_string_ostream error_stream(error_str);

        if (llvm::verifyModule(*module, &error_stream))
        {
            error_stream.flush();
            throw_error("Module verification failed: " + error_str);
        }

        // Create the pass manager that will run the optimization passes on the module.
        llvm::PassBuilder pass_builder;
        llvm::LoopAnalysisManager loop_am;
        llvm::FunctionAnalysisManager function_am;
        llvm::CGSCCAnalysisManager cgsccam;
        llvm::ModuleAnalysisManager module_am;

        pass_builder.registerModuleAnalyses(module_am);
        pass_builder.registerCGSCCAnalyses(cgsccam);
        pass_builder.registerFunctionAnalyses(function_am);
        pass_builder.registerLoopAnalyses(loop_am);
        pass_builder.crossRegisterProxies(loop_am, function_am, cgsccam, module_am);

        llvm::ModulePassManager mpm =
                            pass_builder.buildPerModuleDefaultPipeline(llvm::OptimizationLevel::O3);

        // Now, run the optimization passes on the module.
        mpm.run(*module, module_am);

        // Uncomment to print out the LLVM IR for the JITed function module after being
        // optimized.
        // module->print(llvm::outs(), nullptr);

        // Capture Optimized IR for Each Function...
        for (auto &function : module->functions())
        {
            if (!function.isDeclaration())
            {
                std::string func_ir;
                llvm::raw_string_ostream rso(func_ir);
                function.print(rso);
                rso.flush();

                ir_map[function.getName().str()] = func_ir;
            }
        }

        // Commit our module to the JIT engine and let it get compiled.
        auto error = jit->addIRModule(llvm::orc::ThreadSafeModule(std::move(module),
                                                                  std::move(context)));

        if (error)
        {
            std::string error_message;

            llvm::raw_string_ostream stream(error_message);
            llvm::logAllUnhandledErrors(std::move(error),
                                        stream,
                                        "Failed to jit module: ");
            stream.flush();

            throw_error(error_message);
        }

        return ir_map;
    }


    // JIT compile the given byte-code block into a native function handler.
    std::tuple<std::vector<source::Location>,
                std::vector<run_time::Value>>
                                  Jit::jit_compile_word(run_time::CompilerRuntime& interpreter,
                                                        std::unique_ptr<llvm::Module>& module,
                                                        std::unique_ptr<llvm::LLVMContext>& context,
                                                        const std::string& name,
                                                        const ByteCode& code,
                                                        CodeGenType type)
    {
        // Create the instruction builder for the JITed code.
        llvm::IRBuilder<> builder(*context.get());

        // Gather up our basic types, and then create the function type for the JITed code.
        auto void_type = llvm::Type::getVoidTy(*context.get());
        auto int8_type = llvm::Type::getInt8Ty(*context.get());
        auto ptr_type = llvm::PointerType::getUnqual(int8_type);

        auto function_type = llvm::FunctionType::get(void_type,
                                                        { ptr_type, ptr_type, ptr_type },
                                                        false);

        // Now, create the function that will hold the JITed code.
        auto function = llvm::Function::Create(function_type,
                                                llvm::Function::ExternalLinkage,
                                                name,
                                                module.get());

        // Keep track of source locations found in the byte-code so that we can later keep track of
        // where exceptions occurred.
        std::vector<source::Location> locations;

        // Keep track of constant values that are to complex to be inlined in the generated code.
        std::vector<run_time::Value> constants;

        // Generate the body of the JITed function.
        generate_function_body(interpreter,
                                module,
                                builder,
                                function,
                                context,
                                name,
                                locations,
                                constants,
                                code);

        return { std::move(locations), std::move(constants) };
    }


    run_time::WordHandler Jit::create_word_function(const std::string& name,
                                                    std::string&& function_ir,
                                                    std::vector<source::Location>&& locations_,
                                                    std::vector<run_time::Value>&& constants_,
                                                    CodeGenType type)
    {
        auto locations = std::move(locations_);
        auto constants = std::move(constants_);

        // Get our generated function from the jit engine.
        auto symbol = jit->lookup(name);

        if (!symbol)
        {
            throw_error("Failed to find JITed function symbol " + name + ".");
        }

        auto compiled_function = symbol->toPtr<void (*)(void*, // interpreter
                                                        const void*, // locations
                                                        const void*)>(); // constants


        // Create the function that will hold the JITed code and return it to the caller.
        auto handler_wrapper = [=, this](run_time::CompilerRuntime& runtime) -> void
            {
                // The interpreter pointer is passed in as the first argument.  Convert it
                // to a void pointer for the JITed code.  We do the same for the locations
                // and constants vectors.
                void* runtime_ptr = static_cast<void*>(&runtime);
                const void* locations_ptr = static_cast<const void*>(&locations);
                const void* constants_ptr = static_cast<const void*>(&constants);

                // Clear any previous exception that may have occurred.
                clear_last_exception();

                // Mark our word's context for it's local variables.  But only if we're
                // compiling a word, not a script.
                if (type == CodeGenType::word)
                {
                    runtime.mark_context();
                }

                // Call the JITed function.
                compiled_function(runtime_ptr, locations_ptr, constants_ptr);

                // Release the context, freeing up any local variables.
                if (type == CodeGenType::word)
                {
                    runtime.release_context();
                }

                // If an exception occurred in the JITed code, re-throw it.
                if (last_exception)
                {
                    throw *last_exception;
                }
            };

        // Return the new function handler.
        return handler_wrapper;
    }


    // Generate the body of the JITed function.  We do this by walking the byte-code block and
    // generating the appropriate llvm instructions for each operation.
    void Jit::generate_function_body(run_time::CompilerRuntime& runtime,
                                     std::unique_ptr<llvm::Module>& module,
                                     llvm::IRBuilder<>& builder,
                                     llvm::Function* function,
                                     std::unique_ptr<llvm::LLVMContext>& context,
                                     const std::string& name,
                                     std::vector<source::Location>& locations,
                                     std::vector<run_time::Value>& constants,
                                     const ByteCode& code)
    {
        // Gather some types we'll need.
        auto bool_type = llvm::Type::getInt1Ty(*context.get());
        auto int64_type = llvm::Type::getInt64Ty(*context.get());
        auto double_type = llvm::Type::getDoubleTy(*context.get());
        auto char_type = llvm::Type::getInt1Ty(*context.get());


        // Keep track of the basic blocks we create for the jump targets.
        auto blocks = std::unordered_map<size_t, llvm::BasicBlock*>();
        auto auto_jump_blocks = std::unordered_map<size_t, std::pair<llvm::BasicBlock*,
                                                                        llvm::BasicBlock*>>();

        // Create the entry block of the function.
        auto entry_block = llvm::BasicBlock::Create(*context, "entry_block", function);
        builder.SetInsertPoint(entry_block);

        // Get a reference to the interpreter pointer argument.
        auto runtime_ptr = function->getArg(0);
        auto location_arr_ptr = function->getArg(1);
        auto constant_arr_ptr = function->getArg(2);

        // Keep track of any loop and catch block markers.
        std::vector<std::pair<size_t, size_t>> loop_markers;
        std::vector<size_t> catch_markers;

        // Keep track of any jump targets that are the target of a catch block.
        std::set<size_t> catch_target_markers;


        // Take a first pass through the code to create the blocks that we'll need.
        auto block_index = 1;

        for (size_t i = 0; i < code.size(); ++i)
        {
            if (   (code[i].get_id() == byte_code::Instruction::Id::execute)
                || (code[i].get_id() == byte_code::Instruction::Id::jump_target)
                || (code[i].get_id() == byte_code::Instruction::Id::def_variable)
                || (code[i].get_id() == byte_code::Instruction::Id::def_constant)
                || (code[i].get_id() == byte_code::Instruction::Id::read_variable)
                || (code[i].get_id() == byte_code::Instruction::Id::write_variable)
                || (code[i].get_id() == byte_code::Instruction::Id::jump_loop_start)
                || (code[i].get_id() == byte_code::Instruction::Id::jump_loop_exit))
            {
                // These instructions are the ones that can start a new block.  In the case
                // of a jump target it is an explicit start of a block.  The jump
                // instruction is also the implicit end of a block.  For  the other
                // instructions it is an implicit start of a block.  Usually for exception
                // checking these instructions can call a function that can potentially
                // raise an exception.  So we create a new block for each of these
                // instructions.
                std::stringstream stream;

                stream << "block_" << block_index;
                ++block_index;

                blocks[i] = llvm::BasicBlock::Create(*context, stream.str(), function);
            }
            else if (   (code[i].get_id() == byte_code::Instruction::Id::jump_if_zero)
                     || (code[i].get_id() == byte_code::Instruction::Id::jump_if_not_zero))
            {
                // These instructions have two jumps.  One jump for the pop error check and
                // one for the actual jump.  So we create two blocks for each of these jump
                // instructions.
                std::stringstream stream;

                stream << "block_" << block_index;
                ++block_index;

                auto block_a = llvm::BasicBlock::Create(*context, stream.str(), function);

                std::stringstream stream_b;

                stream_b << "block_" << block_index;
                ++block_index;

                auto block_b = llvm::BasicBlock::Create(*context, stream_b.str(), function);

                auto_jump_blocks[i] = { block_a, block_b };
            }
        }

        // // Create the end block of the function.
        auto exit_block = llvm::BasicBlock::Create(*context, "exit_block", function);

        // On second pass generate the actual code.
        for (size_t i = 0; i < code.size(); ++i)
        {
            // Check to see if the current instruction has a location associated with it.
            if (code[i].get_location())
            {
                // Save the location in our locations vector.
                auto& location = code[i].get_location().value();
                auto index = locations.size();

                locations.push_back(location);

                // Generate the code to update the location in the interpreter.
                auto index_const = llvm::ConstantInt::get(int64_type, index);
                builder.CreateCall(handle_set_location_fn,
                                    { runtime_ptr, location_arr_ptr, index_const});
            }

            switch (code[i].get_id())
            {
                case byte_code::Instruction::Id::def_variable:
                    {
                        // Get the variable name and create a string constant in the module
                        // for it.
                        auto name = code[i].get_value().get_string(runtime);
                        auto name_ptr = define_string_constant(name,
                                                                builder,
                                                                module,
                                                                context);

                        // Call the handle_define_variable function to define the variable
                        // in the interpreter.
                        auto result = builder.CreateCall(handle_define_variable_fn,
                                                            { runtime_ptr, name_ptr });

                        // Check the result of the call instruction and branch to the next
                        // block if no errors were raised, otherwise branch to the either
                        // the exit block or the exception handler block.
                        auto error_block = catch_markers.empty()
                                            ? exit_block
                                            : blocks[catch_markers.back()];

                        auto cmp = builder.CreateICmpNE(result, builder.getInt64(0));
                        builder.CreateCondBr(cmp, error_block, blocks[i]);
                        builder.SetInsertPoint(blocks[i]);
                    }
                    break;

                case byte_code::Instruction::Id::def_constant:
                    {
                        // Get the constant name and create a string constant in the module
                        // for it.
                        auto name = code[i].get_value().get_string(runtime);
                        auto name_ptr = define_string_constant(name,
                                                                builder,
                                                                module,
                                                                context);

                        // Call the handle_define_constant function to define the constant
                        // in the interpreter.
                        auto result = builder.CreateCall(handle_define_constant_fn,
                                                            { runtime_ptr, name_ptr });

                        // Check the result of the call instruction and branch to the next
                        // block if no errors were raised, otherwise branch to the either
                        // the exit block or the exception handler block.
                        auto error_block = catch_markers.empty()
                                            ? exit_block
                                            : blocks[catch_markers.back()];

                        auto cmp = builder.CreateICmpNE(result, builder.getInt64(0));
                        builder.CreateCondBr(cmp, error_block, blocks[i]);
                        builder.SetInsertPoint(blocks[i]);
                    }
                    break;

                case byte_code::Instruction::Id::read_variable:
                    {
                        // Call the handle_read_variable function to read the variable from
                        // the interpreter.
                        auto result = builder.CreateCall(handle_read_variable_fn,
                                                            { runtime_ptr });

                        // Check the result of the call instruction and branch to the next
                        // block if no errors were raised, otherwise branch to the either
                        // the exit block or the exception handler block.
                        auto error_block = catch_markers.empty()
                                            ? exit_block
                                            : blocks[catch_markers.back()];

                        auto cmp = builder.CreateICmpNE(result, builder.getInt64(0));
                        builder.CreateCondBr(cmp, error_block, blocks[i]);
                        builder.SetInsertPoint(blocks[i]);
                    }
                    break;

                case byte_code::Instruction::Id::write_variable:
                    {
                        // Call the handle_write_variable function to write the variable to
                        // the interpreter.
                        auto result = builder.CreateCall(handle_write_variable_fn,
                                                            { runtime_ptr });

                        // Check the result of the call instruction and branch to the next
                        // block if no errors were raised, otherwise branch to the either
                        // the exit block or the exception handler block.
                        auto error_block = catch_markers.empty()
                                            ? exit_block
                                            : blocks[catch_markers.back()];

                        auto cmp = builder.CreateICmpNE(result, builder.getInt64(0));
                        builder.CreateCondBr(cmp, error_block, blocks[i]);
                        builder.SetInsertPoint(blocks[i]);
                    }
                    break;

                case byte_code::Instruction::Id::execute:
                    {
                        // Get the name or index of the word to execute.
                        auto& value = code[i].get_value();

                        // Capture the result of teh execute call instruction.
                        llvm::CallInst* result = nullptr;

                        if (value.is_string())
                        {
                            // The value is a string, so execute the word based on it's
                            // name.
                            auto name = value.get_string(runtime);
                            auto name_ptr = define_string_constant(name,
                                                                    builder,
                                                                    module,
                                                                    context);

                            result = builder.CreateCall(handle_word_execute_name_fn,
                                                        { runtime_ptr, name_ptr });
                        }
                        else if (value.is_numeric())
                        {
                            // The value is a number, so execute the word based on it's
                            // handler index.
                            auto index = value.get_int(runtime);
                            auto int_const = llvm::ConstantInt::get(int64_type, index);
                            result = builder.CreateCall(handle_word_execute_index_fn,
                                                        { runtime_ptr, int_const });
                        }
                        else
                        {
                            throw_error("Can not execute unexpected value type.");
                        }

                        // Check the result of the call instruction and branch to the next
                        // if no errors were raised, otherwise branch to the either the
                        // exit block or the exception handler block.
                        auto next_block = blocks[i];

                        // Jump to the catch block if there is one, or the exit block if
                        // not.
                        auto error_block = catch_markers.empty()
                                            ? exit_block
                                            : blocks[catch_markers.back()];

                        auto cmp = builder.CreateICmpNE(result, builder.getInt64(0));
                        builder.CreateCondBr(cmp, error_block, next_block);

                        // We're done with the current block, so move on to the next one.
                        builder.SetInsertPoint(next_block);
                    }
                    break;

                case byte_code::Instruction::Id::word_index:
                    {
                        // Make sure we have a name for lookup.
                        if (!code[i].get_value().is_string())
                        {
                            throw_error("Expected a numeric value for word_index.");
                        }

                        // See if we can find the word in the interpreter at compile time.
                        // If we can, we'll generate the code to push it's index on the
                        // stack, if we can't we'll generate code to try again at runtime.
                        auto name = code[i].get_value().get_string(runtime);
                        auto [ found, word_info ] = runtime.find(name);

                        if (found)
                        {
                            auto int_const = llvm::ConstantInt::get(int64_type,
                                                                    word_info.get_handler_index());
                            builder.CreateCall(handle_push_int_fn,
                                                { runtime_ptr, int_const });
                        }
                        else
                        {
                            auto name_constant = define_string_constant(name,
                                                                        builder,
                                                                        module,
                                                                        context);
                            builder.CreateCall(handle_word_index_name_fn,
                                               { runtime_ptr, name_constant });
                        }
                    }
                    break;

                case byte_code::Instruction::Id::word_exists:
                    {
                        // Make sure we have a name for lookup.
                        if (!code[i].get_value().is_string())
                        {
                            throw_error("Expected a numeric value for word_index.");
                        }

                        // See if we can find the word in the interpreter at compile time.
                        // If we can, we'll generate the code to push a true value onto the
                        // stack, if we can't we'll generate code to try again at runtime.
                        auto name = code[i].get_value().get_string(runtime);
                        auto [ found, _ ] = runtime.find(name);

                        if (found)
                        {
                            auto true_const = llvm::ConstantInt::get(bool_type, true);
                            builder.CreateCall(handle_push_bool_fn,
                                                { runtime_ptr, true_const });
                        }
                        else
                        {
                            auto name_ptr = define_string_constant(name,
                                                                    builder,
                                                                    module,
                                                                    context);

                            builder.CreateCall(handle_word_exists_name_fn,
                                               { runtime_ptr, name_ptr });
                        }
                    }
                    break;

                case byte_code::Instruction::Id::push_constant_value:
                    {
                        // Get the value to push from the instruction.
                        auto& value = code[i].get_value();

                        // Check the type of the value.  If it's one of the simple types
                        // directly generate the code to push it's value onto the stack.
                        if (value.is_bool())
                        {
                            auto bool_value = value.get_bool();
                            auto bool_const = llvm::ConstantInt::get(bool_type, bool_value);
                            builder.CreateCall(handle_push_bool_fn,
                                                { runtime_ptr, bool_const });
                        }
                        else if (value.is_int())
                        {
                            auto int_value = value.get_int();
                            auto int_const = llvm::ConstantInt::get(int64_type, int_value);
                            builder.CreateCall(handle_push_int_fn,
                                                { runtime_ptr, int_const });
                        }
                        else if (value.is_double())
                        {
                            auto double_value = value.get_double();
                            auto double_const = llvm::ConstantFP::get(double_type,
                                                                        double_value);
                            builder.CreateCall(handle_push_double_fn,
                                                { runtime_ptr, double_const });
                        }
                        else if (value.is_string())
                        {
                            auto string_value = value.get_string();
                            auto string_ptr = define_string_constant(string_value,
                                                                        builder,
                                                                        module,
                                                                        context);
                            builder.CreateCall(handle_push_string_fn,
                                                { runtime_ptr, string_ptr });
                        }
                        else
                        {
                            // The value is a complex type that we can't inline in the JITed
                            // code directly.  So we'll add it to the constants array and
                            // generate code to push the value from the array onto the
                            // stack.
                            auto index = constants.size();
                            constants.push_back(value);

                            auto index_const = builder.getInt64(index);
                            builder.CreateCall(handle_push_value_fn,
                                                { runtime_ptr,
                                                    constant_arr_ptr,
                                                    index_const });
                        }
                    }
                    break;

                case byte_code::Instruction::Id::mark_loop_exit:
                    {
                        // Capture the start and end indexes of the loop for later use.
                        auto start_index = i + 1;
                        auto end_index = i + code[i].get_value().get_int(runtime);

                        loop_markers.push_back({ start_index, end_index });
                    }
                    break;

                case byte_code::Instruction::Id::unmark_loop_exit:
                    {
                        // Clear the current loop markers.
                        loop_markers.pop_back();
                    }
                    break;

                case byte_code::Instruction::Id::mark_catch:
                    {
                        // Capture the index of the catch block for later use.
                        auto target_index = i + code[i].get_value().get_int(runtime);

                        catch_markers.push_back(target_index);
                        catch_target_markers.insert(target_index);
                    }
                    break;

                case byte_code::Instruction::Id::unmark_catch:
                    {
                        // Clear the current catch markers so that we don't jump to then.
                        // Note that we leave the catch_target_markers set alone so that we
                        // can generate the code to push the exception onto the stack when
                        // we reach the catch target instruction.
                        catch_markers.pop_back();
                    }
                    break;

                case byte_code::Instruction::Id::mark_context:
                    {
                        // Call the handle_manage_context function to mark the context in
                        // the interpreter.
                        auto bool_const = llvm::ConstantInt::get(bool_type, true);
                        builder.CreateCall(handle_manage_context_fn,
                                           { runtime_ptr, bool_const });
                    }
                    break;

                case byte_code::Instruction::Id::release_context:
                    {
                        // Call the handle_manage_context function to release the context in
                        // the interpreter.
                        auto bool_const = llvm::ConstantInt::get(bool_type, false);
                        builder.CreateCall(handle_manage_context_fn,
                                           { runtime_ptr, bool_const });
                    }
                    break;

                case byte_code::Instruction::Id::jump:
                    {
                        // Jump to the target block.
                        auto index = i + code[i].get_value().get_int(runtime);
                        builder.CreateBr(blocks[index]);
                    }
                    break;

                case byte_code::Instruction::Id::jump_if_zero:
                    {
                        // Convert the relative index to an absolute index.
                        auto index = i + code[i].get_value().get_int(runtime);

                        // Allocate a bool to hold the test value.  Then call the
                        // handle_pop_bool function to get the value from the stack.
                        auto test_value = builder.CreateAlloca(bool_type);
                        auto pop_result = builder.CreateCall(handle_pop_bool_fn,
                                                             { runtime_ptr, test_value });

                        // Check the result of the call instruction and branch to the next
                        // block if no errors were raised, otherwise branch to the either
                        // the exit block or the exception handler block.
                        auto error_block = catch_markers.empty()
                                            ? exit_block
                                            : blocks[catch_markers.back()];

                        // Get the two blocks that we'll jump to based on the test value.
                        auto [ a, b ] = auto_jump_blocks[i];

                        auto cmp = builder.CreateICmpNE(pop_result, builder.getInt64(0));
                        builder.CreateCondBr(cmp, error_block, a);

                        // The pop was successful, so switch to the next block and generate
                        // the code to perform the jump based on the test value.
                        builder.SetInsertPoint(a);

                        // Jump to the 'success' block if the test value is false, otherwise
                        // jump to the 'fail' block.
                        auto read_value = builder.CreateLoad(bool_type, test_value);
                        builder.CreateCondBr(read_value, b, blocks[index]);
                        builder.SetInsertPoint(b);
                    }
                    break;

                case byte_code::Instruction::Id::jump_if_not_zero:
                    {
                        // Convert the relative index to an absolute index.
                        auto index = i + code[i].get_value().get_int(runtime);

                        // Allocate a bool to hold the test value.  Then call the
                        // handle_pop_bool function to get the value from the stack.
                        auto test_value = builder.CreateAlloca(bool_type);
                        auto pop_result = builder.CreateCall(handle_pop_bool_fn,
                                                            { runtime_ptr, test_value });

                        // Check the result of the call instruction and branch to the next
                        // block if no errors were raised, otherwise branch to the either
                        // the exit block or the exception handler block.
                        auto error_block = catch_markers.empty()
                                            ? exit_block
                                            : blocks[catch_markers.back()];

                        auto [ a, b ] = auto_jump_blocks[i];

                        auto cmp = builder.CreateICmpNE(pop_result, builder.getInt64(0));
                        builder.CreateCondBr(cmp, error_block, a);

                        // The pop was successful, so switch to the next block and generate
                        // the code to perform the jump based on the test value.
                        builder.SetInsertPoint(a);

                        // Jump to the 'success' block if the test value is true, otherwise
                        // jump to the 'fail' block.
                        auto read_value = builder.CreateLoad(bool_type, test_value);
                        builder.CreateCondBr(read_value, blocks[index], b);
                        builder.SetInsertPoint(b);
                    }
                    break;

                case byte_code::Instruction::Id::jump_loop_start:
                    {
                        // Jump to the start block of the loop.
                        auto start_index = loop_markers.back().first;

                        builder.CreateBr(blocks[start_index]);
                        builder.SetInsertPoint(blocks[i]);
                    }
                    break;

                case byte_code::Instruction::Id::jump_loop_exit:
                    {
                        // Jump to the end block of the loop.
                        auto end_index = loop_markers.back().second;
                        builder.CreateBr(blocks[end_index]);

                        builder.SetInsertPoint(blocks[i]);
                    }
                    break;

                case byte_code::Instruction::Id::jump_target:
                    {
                        // Make sure that the current block has a terminator instruction, if
                        // not, add one to jump to the next block.  This is because this
                        // would be a natural follow through in the original byte-code.
                        if (builder.GetInsertBlock()->getTerminator() == nullptr)
                        {
                            builder.CreateBr(blocks[i]);
                        }

                        // We're done with the current block, switch over to the next.
                        builder.SetInsertPoint(blocks[i]);


                        // If this is a catch target, we need to generate the code to push
                        // the exception onto the stack and clear the last exception.
                        if (   (!catch_target_markers.empty())
                            && (catch_target_markers.find(i) != catch_target_markers.end()))
                        {
                            builder.CreateCall(handle_push_last_exception_fn,
                                                { runtime_ptr });
                        }
                    }
                    break;
            }
        }

        // Make sure that the last block has a terminator instruction, if not, add one to
        // jump to the exit block.
        if (builder.GetInsertBlock()->getTerminator() == nullptr)
        {
            builder.CreateBr(exit_block);
        }

        // We're done with the last user code block, so switch over to the exit block and
        // generate the code to return from the function.
        builder.SetInsertPoint(exit_block);
        builder.CreateRetVoid();
    }


    // Define a new string constant with the given text in the JIT module.
    llvm::Value* Jit::define_string_constant(const std::string& text,
                                             llvm::IRBuilder<>& builder,
                                             std::unique_ptr<llvm::Module>& module,
                                             std::unique_ptr<llvm::LLVMContext>& context)
    {
        // Get our types.
        auto char_type = llvm::Type::getInt8Ty(*context.get());
        auto char_ptr_type = llvm::PointerType::getUnqual(char_type);

        // Create the constant data array for the string then create a global variable to
        // hold it.
        auto string_constant = llvm::ConstantDataArray::getString(*context.get(),
                                                                    text,
                                                                    true);

        auto global = new llvm::GlobalVariable(*module,
                                                string_constant->getType(),
                                                true,
                                                llvm::GlobalValue::PrivateLinkage,
                                                string_constant);

        // Create a pointer to the global variable for the string constant.
        llvm::Value* string_ptr = builder.CreatePointerCast(global, char_ptr_type);

        return string_ptr;
    }


    // Handle setting the current source location in the runtime for the JITed code.
    void Jit::handle_set_location(void* runtime_ptr, void* location_array_ptr, size_t index)
    {
        auto& runtime = *static_cast<run_time::CompilerRuntime*>(runtime_ptr);
        auto& location_array = *static_cast<std::vector<source::Location>*>(location_array_ptr);
        auto& location = location_array[index];

        runtime.set_location(location);
    }


    // Handle the context management for the JITed code.  If passed a true value, mark the
    // context, otherwise release it.
    void Jit::handle_manage_context(void* runtime_ptr, bool is_marking)
    {
        auto& runtime = *static_cast<run_time::CompilerRuntime*>(runtime_ptr);

        // TODO: Add error checking to make sure that these calls are balanced.

        if (is_marking)
        {
            runtime.mark_context();
        }
        else
        {
            runtime.release_context();
        }
    }


    // Handle defining a new variable in the runtime for the JITed code.
    int64_t Jit::handle_define_variable(void* runtime_ptr, const char* name)
    {
        int64_t result = 0;

        try
        {
            auto& runtime = *static_cast<run_time::CompilerRuntime*>(runtime_ptr);

            runtime.define_variable(name);
        }
        catch (std::runtime_error& error)
        {
            set_last_exception(error);
            result = -1;
        }

        return result;
    }


    // Handle defining a new constant in the runtime for the JITed code.
    int64_t Jit::handle_define_constant(void* runtime_ptr, const char* name)
    {
        int64_t result = 0;

        try
        {
            auto& runtime = *static_cast<run_time::CompilerRuntime*>(runtime_ptr);
            auto value = runtime.pop();

            runtime.define_constant(name, value);
        }
        catch (std::runtime_error& error)
        {
            set_last_exception(error);
            result = -1;
        }

        return result;
    }


    // Handle a variable read operation for the JITed code.
    int64_t Jit::handle_read_variable(void* runtime_ptr)
    {
        int64_t result = 0;

        try
        {
            auto& runtime = *static_cast<run_time::CompilerRuntime*>(runtime_ptr);
            auto index = runtime.pop_as_size();

            runtime.push(runtime.read_variable(index));
        }
        catch (std::runtime_error& error)
        {
            set_last_exception(error);
            result = -1;
        }

        return result;
    }


    // Handle a variable write operation for the JITed code.
    int64_t Jit::handle_write_variable(void* runtime_ptr)
    {
        int64_t result = 0;

        try
        {
            auto& runtime = *static_cast<run_time::CompilerRuntime*>(runtime_ptr);
            auto index = runtime.pop_as_size();
            auto value = runtime.pop();

            runtime.write_variable(index, value);
        }
        catch (std::runtime_error& error)
        {
            set_last_exception(error);
            result = -1;
        }

        return result;
    }


    void Jit::handle_push_last_exception(void* runtime_ptr)
    {
        auto& runtime = *static_cast<run_time::CompilerRuntime*>(runtime_ptr);

        if (last_exception)
        {
            // Push the exception message onto the runtime stack, and clear the last
            // exception because user code has handled it.
            runtime.push(last_exception->what());
            clear_last_exception();
        }
        else
        {
            runtime.push("");
        }
    }


    // Handle popping a boolean value from the runtime stack for the JITed code.
    int64_t Jit::handle_pop_bool(void* runtime_ptr, bool* value)
    {
        int64_t result = 0;

        try
        {
            auto& runtime  = *static_cast<run_time::CompilerRuntime*>(runtime_ptr);

            *value = runtime.pop_as_bool();
        }
        catch (std::runtime_error& error)
        {
            set_last_exception(error);
            result = -1;
        }

        return result;
    }


    // Handle pushing a boolean value onto the runtime stack for the JITed code.
    void Jit::handle_push_bool(void* runtime_ptr, bool value)
    {
        auto& runtime = *static_cast<run_time::CompilerRuntime*>(runtime_ptr);

        runtime.push(value);
    }


    // Handle pushing an integer value onto the runtime stack for the JITed code.
    void Jit::handle_push_int(void* runtime_ptr, int64_t value)
    {
        auto& runtime = *static_cast<run_time::CompilerRuntime*>(runtime_ptr);

        runtime.push(value);
    }


    // Handle pushing a double value onto the runtime stack for the JITed code.
    void Jit::handle_push_double(void* runtime_ptr, double value)
    {
        auto& runtime = *static_cast<run_time::CompilerRuntime*>(runtime_ptr);

        runtime.push(value);
    }


    // Handle pushing a string value onto the runtime stack for the JITed code.
    void Jit::handle_push_string(void* runtime_ptr, const char* value)
    {
        auto& runtime = *static_cast<run_time::CompilerRuntime*>(runtime_ptr);

        runtime.push(std::string(value));
    }


    void Jit::handle_push_value(void* runtime_ptr, void* array_ptr, int64_t index)
    {
        auto& runtime = *static_cast<run_time::CompilerRuntime*>(runtime_ptr);
        auto& array = *static_cast<std::vector<run_time::Value>*>(array_ptr);
        auto& value = array[index];

        runtime.push(value);
    }


    // Handle executing a word by name for the JITed code.
    int64_t Jit::handle_word_execute_name(void* runtime_ptr, const char* name)
    {
        int64_t result = 0;
        auto& runtime = *static_cast<run_time::CompilerRuntime*>(runtime_ptr);

        try
        {
            runtime.execute(name);
        }
        catch (std::runtime_error& error)
        {
            set_last_exception(error);
            result = -1;
        }

        return result;
    }


    // Handle executing a word by index for the JITed code.
    int64_t Jit::handle_word_execute_index(void* runtime_ptr, int64_t index)
    {
        int64_t result = 0;
        auto& runtime = *static_cast<run_time::CompilerRuntime*>(runtime_ptr);

        try
        {
            runtime.execute(static_cast<size_t>(index));
        }
        catch (std::runtime_error& error)
        {
            set_last_exception(error);
            result = -1;
        }

        return result;
    }


    // Handle looking up a word index by name for the JITed code.
    int64_t Jit::handle_word_index_name(void* runtime_ptr, const char* name)
    {
        int64_t result = 0;

        try
        {
            auto& runtime = *static_cast<run_time::CompilerRuntime*>(runtime_ptr);
            auto [ found, word ] = runtime.find(name);

            if (!found)
            {
                throw_error(runtime, "Word '" + std::string(name) + "' not found.");
            }

            runtime.push((int64_t)word.get_handler_index());
        }
        catch (std::runtime_error& error)
        {
            set_last_exception(error);
            result = -1;
        }

        return result;
    }


    // Handle looking up a word index by name for the JITed code.
    int64_t Jit::handle_word_exists_name(void* runtime_ptr, const char* name)
    {
        int64_t result = 0;

        try
        {
            auto& runtime = *static_cast<run_time::CompilerRuntime*>(runtime_ptr);
            auto [ found, _ ] = runtime.find(name);

            runtime.push(found);
        }
        catch (std::runtime_error& error)
        {
            set_last_exception(error);
            result = -1;
        }

        return result;
    }


    // Set the last exception that occurred.
    void Jit::set_last_exception(const std::runtime_error& error)
    {
        last_exception = error;
    }


    // Clear the last exception that occurred.
    void Jit::clear_last_exception()
    {
        last_exception.reset();
    }


}

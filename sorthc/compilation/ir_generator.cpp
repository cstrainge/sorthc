
#include "sorthc.h"



namespace sorth::compilation
{


    void generate_llvm_ir(const byte_code::ScriptPtr& script, const std::filesystem::path& output_path)
    {
        llvm::LLVMContext context;

        auto module = std::make_shared<llvm::Module>(script->get_script_path().string(), context);

        llvm::IRBuilder<> builder(context);

        // Find all the words in the script and it's subscripts, generate an unique index for each
        // word.

        // Eliminate all words that are not used by any of the top-levels.

        // Resolve all calls to their proper indicies.

        // Resolve variable references to their proper indicies or turn into direct calls.

        // Create functions for all remaining words.


        // Create the script top-level function.  This function will be the entry point for the
        // resulting program.  It'll be made of of all the top level blocks in each of the scripts.

        // Start off by creating the signature for the top-level function.
        llvm::FunctionType* top_level_signature =
                                             llvm::FunctionType::get(llvm::Type::getInt8Ty(context),
                                                                     false);

        // Create the function itself.
        llvm::Function* top_level_function = llvm::Function::Create(top_level_signature,
                                                                    llvm::Function::ExternalLinkage,
                                                                    "script_top_level",
                                                                    module.get());

        // Create the entry block for the top-level function and set the builder to insert code into
        // it.
        auto entry_block = llvm::BasicBlock::Create(context, "entry", top_level_function);

        builder.SetInsertPoint(entry_block);

        // Generate the return statement for the top-level function.
        auto ret_val = llvm::ConstantInt::get(builder.getInt8Ty(), EXIT_FAILURE);
        builder.CreateRet(ret_val);

        // Verify that the module is well-formed.
        //std::cout << "Generated module:" << std::endl;
        //module->print(llvm::outs(), nullptr);

        if (verifyModule(*module, &llvm::errs()))
        {
            throw_error("Generated LLVM IR module is invalid.");
        }


        // Write the LLVM IR to an object file.

        auto target_triple = llvm::sys::getDefaultTargetTriple();

        std::string error;
        const auto target = llvm::TargetRegistry::lookupTarget(target_triple, error);

        if (!target)
        {
            throw_error("Failed to lookup LLVM target: " + error);
        }

        llvm::TargetOptions options;

        auto reloc_model = std::optional<llvm::Reloc::Model>();
        auto target_machine = target->createTargetMachine(target_triple,
                                                          "generic",
                                                          "",
                                                          options,
                                                          reloc_model);

        module->setDataLayout(target_machine->createDataLayout());
        module->setTargetTriple(target_triple);

        module->print(llvm::outs(), nullptr);

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


#pragma once



namespace sorth::compilation
{


    // Generate the LLVM IR for a script and it's sub-scripts, and write the resulting IR to an
    // object file.
    void generate_llvm_ir(const byte_code::ScriptPtr& standard_library,
                          const byte_code::ScriptPtr& script,
                          const std::filesystem::path& output_path);


}


#pragma once



namespace sorth::compilation
{


    void generate_llvm_ir(const byte_code::ScriptPtr& script,
                          const std::filesystem::path& output_path);


}

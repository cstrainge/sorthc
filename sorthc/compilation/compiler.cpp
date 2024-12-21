
#include "sorthc.h"



namespace sorthc::compilation
{


    Compiler::Compiler(const std::filesystem::path& system_path) noexcept
    : runtime(system_path)
    {
    }


    void Compiler::append_search_path(const std::filesystem::path& path)
    {
        runtime.append_search_path(path);
    }


    void Compiler::compile(const std::filesystem::path& source_path,
                           const std::filesystem::path& output_path)
    {
        // Ask the runtime to byte-code compile the script.
        // runtime.compile_script(source_path);

        // Now that the script is compiled, we can generate the LLVM IR for it.
        // generate_llvm_ir(runtime.get_script_cache(), source_path);

        // Finally, we can compile the LLVM IR into native code.\
        // compile_native_code(output_path);
    }


}


#include "sorthc.h"



namespace sorth::compilation
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
        auto script = runtime.compile_script(source_path);

        for (const auto& word : script->get_words())
        {
            std::cout << "Word: " << word.get_name() << std::endl
                      << pretty_print(runtime, word.get_code())
                      << std::endl;
        }

        std::cout << "<top-level>" << std::endl
                  << pretty_print(runtime, script->get_top_level())
                  << std::endl;

        // Now that the script and it's sub-scripts are byte-code compiled, we can generate the LLVM
        // IR for it.  Then we can compile  the LLVM IR to native code into an object file that can
        // be linked with the runtime library into an executable.
        generate_llvm_ir(runtime.get_standard_library(), script, output_path);
    }


}

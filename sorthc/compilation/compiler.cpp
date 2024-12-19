
#include "sorthc.h"



namespace sorthc::compilation
{


    Compiler::Compiler(const std::filesystem::path& system_path) noexcept
    : search_paths()
    {
        append_search_path(system_path);
    }


    void Compiler::append_search_path(const std::filesystem::path& path)
    {
    }


    void Compiler::compile(const std::filesystem::path& source_path,
                           const std::filesystem::path& output_path)
    {
        auto cache = runtime.compile_script(search_paths, source_path);
    }


}

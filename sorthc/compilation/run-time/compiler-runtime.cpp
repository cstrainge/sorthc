
#include "sorthc.h"



namespace sorthc::compilation::run_time
{


    CompilerRuntime::CompilerRuntime() noexcept
    : location(LOCATION_HERE)
    {
    }


    const source::Location& CompilerRuntime::get_location() const noexcept
    {
        return location;
    }


    ScriptCache CompilerRuntime::compile_script(const SearchPaths& paths,
                                                const std::filesystem::path& path)
    {
        return {};
    }


}

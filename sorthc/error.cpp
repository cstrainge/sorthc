
#include "sorthc.h"



namespace sorthc
{


    void throw_error(const std::string& message)
    {
        throw std::runtime_error(message);
    }


    void throw_error(const source::Location& location, const std::string& message)
    {
        std::ostringstream stream;

        stream << location << ": Error: " << message;

        throw_error(stream.str());
    }



    void throw_error(const compilation::run_time::CompilerRuntime& runtime,
                     const::std::string& message)
    {
        throw_error(runtime.get_location(), message);
    }


}

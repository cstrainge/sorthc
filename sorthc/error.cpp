
#include "sorthc.h"



namespace sorthc
{


    void throw_error(const std::string& message)
    {
std::cout << "throw_error: " << message << std::endl;
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
        const auto& call_stack = runtime.get_call_stack();

        if (!call_stack.empty())
        {
            std::ostringstream stream;

            stream << message << std::endl
                   << std::endl
                   << "Call stack:" << std::endl;

            for (const auto& entry : call_stack)
            {
                stream << "  " << entry.location << ": " << entry.name << std::endl;
            }

            throw_error(runtime.get_location(), stream.str());
        }

        throw_error(runtime.get_location(), message);
    }


}

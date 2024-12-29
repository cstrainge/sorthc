
#pragma once



namespace sorth
{


    namespace source
    {


        class Location;


    }


    namespace compilation::run_time
    {


        class CompilerRuntime;


    }


    [[noreturn]]
    void throw_error(const std::string& message);


    [[noreturn]]
    inline void throw_error(const char* message)
    {
        throw_error(std::string(message));
    }


    [[noreturn]]
    void throw_error(const source::Location& location, const std::string& message);


    [[noreturn]]
    void throw_error(const compilation::run_time::CompilerRuntime& runtime,
                     const::std::string& message);


    inline void throw_error_if(bool condition,
                               const source::Location& location,
                               const std::string& message)
    {
        if (condition)
        {
            throw_error(location, message);
        }
    }


    inline void throw_error_if(bool condition,
                               const compilation::run_time::CompilerRuntime& runtime,
                               const std::string& message)
    {
        if (condition)
        {
            throw_error(runtime, message);
        }
    }


}

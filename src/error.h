
#pragma once



namespace sorthc
{


    [[noreturn]]
    void throw_error(const Location& location, const std::string& message);


    inline void throw_error_if(bool condition, const Location& location, const std::string& message)
    {
        if (condition)
        {
            throw_error(location, message);
        }
    }


}

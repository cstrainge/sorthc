
#include "sorthc.h"



namespace sorthc
{


    void throw_error(const source::Location& location, const std::string& message)
    {
        std::ostringstream stream;

        stream << location << ": Error: " << message;

        throw std::runtime_error(stream.str());
    }


}

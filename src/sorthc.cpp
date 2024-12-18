
#include "sorthc.h"



int main(int argc, char* argv[])
{
    int result = EXIT_SUCCESS;

    try
    {
        //
    }
    catch (const std::exception& error)
    {
        std::cerr << error.what() << std::endl;
        result = EXIT_FAILURE;
    }

    return result;
}


#include "sorth-runtime.h"



extern "C" int script_top_level();



int main(int argc, char* argv[])
{
    int result = EXIT_SUCCESS;

    try
    {
        stack_initialize();

        result = script_top_level();
    }
    catch(const std::exception& error)
    {
        std::cerr << "An unexpected error occured: " << std::endl
                  << error.what() << std::endl;
    }

    return result;
}


#include "sorthc.h"



#if (IS_APPLE == 1)

    #include <assert.h>
    #include <mach-o/dyld.h>

#elif (IS_UNIX == 1)

    #include <linux/limits.h>
    #include <unistd.h>

#elif (IS_WINDOWS == 1)

    #include <windows.h>

#endif

// Disable the CRT secure warnings for the deprecation warning for the function std::get_env on
// Windows.
#define _CRT_SECURE_NO_WARNINGS 1
#include <cstdlib>
#undef _CRT_SECURE_NO_WARNINGS



namespace
{


    // Get the directory where the executable is located.  This is used to find the standard library
    // in the case the user hasn't specified another location for it.
    std::filesystem::path get_executable_directory()
    {
        std::filesystem::path base_path;

        // Refer to OS specific means to get the current executable's path.
        #if (IS_APPLE == 1)

            uint32_t buffer_size = 0;

            assert(_NSGetExecutablePath(nullptr, &buffer_size) == -1);

            std::vector<char> buffer((size_t)buffer_size);

            assert(_NSGetExecutablePath(&buffer[0], &buffer_size) == 0);

            base_path = std::filesystem::canonical(&buffer[0]).remove_filename();

        #elif (IS_UNIX == 1)

            char buffer [PATH_MAX + 1];
            ssize_t count = 0;

            memset(buffer, 0, PATH_MAX + 1);

            count = readlink("/proc/self/exe", buffer, PATH_MAX);

            if (count < 0)
            {
                throw std::runtime_error("Executable path could not be read, " +
                                         std::string(strerror(errno)) + ".");
            }

            base_path = std::filesystem::canonical(buffer).remove_filename();

        #elif (IS_WINDOWS == 1)

            char buffer [ MAX_PATH + 1];

            memset(buffer, 0, MAX_PATH + 1);

            GetModuleFileNameA(nullptr, buffer, MAX_PATH);

            if (GetLastError() != ERROR_SUCCESS)
            {
                char message_buffer[4096];
                memset(message_buffer, 0, 4096);
                size_t size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                                             FORMAT_MESSAGE_FROM_SYSTEM |
                                             FORMAT_MESSAGE_IGNORE_INSERTS,
                                             nullptr,
                                             GetLastError(),
                                             MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                                             message_buffer,
                                             0,
                                             NULL);
                std::stringstream stream;

                stream << "Could not get the executable directory: " << message_buffer;

                throw std::runtime_error(stream.str());
            }

            base_path = std::filesystem::canonical(buffer).remove_filename();

        #else

            throw std::runtime_error("get_executable_directory is unimplemented on this platform.");

        #endif

        return base_path;
    }


    // Get the directory where the standard library is located.  This can either be the directory
    // that the executable is in, or it can be specified by the SORTH_LIB environment variable.
    std::filesystem::path get_std_lib_directory()
    {
        auto env_path = std::getenv("SORTH_LIB");

        if (env_path != nullptr)
        {
            return std::filesystem::canonical(env_path);
        }

        return get_executable_directory();
    }
}



int main(int argc, char* argv[])
{
    int result = EXIT_SUCCESS;

    try
    {
        // Create a compiler and compile the source file.
        auto compiler = sorthc::compilation::Compiler(get_std_lib_directory());

        if (argc != 3)
        {
            throw std::runtime_error("Usage: sorthc <source-file> <output-file>");
        }

        // Compile the source file, to the given output file.
        compiler.compile(argv[1], argv[2]);
    }
    catch (const std::exception& error)
    {
        std::cerr << error.what() << std::endl;
        result = EXIT_FAILURE;
    }

    return result;
}

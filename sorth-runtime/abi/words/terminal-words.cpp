
#include "sorth-runtime.h"
#include "terminal-words.h"


#if defined(__APPLE__) || defined(__linux__)

    #include <termios.h>
    #include <unistd.h>
    #include <sys/ioctl.h>

#elif defined(_WIN64)

    #include <windows.h>

#endif



using namespace sorth::run_time::data_structures;


extern "C"
{


    #if defined(__APPLE__) || defined(__linux__)


        // This part requires posix.  If we want to work on another os, this needs to be ported.
        // We're using this to enable and disable the terminal emulator's raw mode.
        struct termios original_termios;
        bool is_in_raw_mode = false;


        uint8_t word_term_raw_mode()
        {
            bool requested_on;

            auto pop_result = stack_pop_bool(&requested_on);

            if (pop_result)
            {
                return 1;
            }

            if (requested_on && (!is_in_raw_mode))
            {
                struct termios raw = original_termios;

                auto result = tcgetattr(STDIN_FILENO, &original_termios);

                if (result == -1)
                {
                    set_last_error(("Could not read terminal mode information, " +
                                    std::string(strerror(errno)) + ".").c_str());

                    return 1;
                }

                raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
                raw.c_oflag &= ~(OPOST);
                raw.c_cflag |= (CS8);
                raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
                raw.c_cc[VMIN] = 1;

                result = tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);

                if (result == -1)
                {
                    set_last_error(("Could not set terminal mode, " +
                                    std::string(strerror(errno)) + ".").c_str());

                    return 1;
                }

                is_in_raw_mode = true;
            }
            else if ((!requested_on) && is_in_raw_mode)
            {
                auto result = tcsetattr(STDIN_FILENO, TCSAFLUSH, &original_termios);

                if (result == -1)
                {
                    set_last_error(("Could not reset terminal mode, " +
                                    std::string(strerror(errno)) + ".").c_str());

                    return 1;
                }

                is_in_raw_mode = false;
            }

            return 0;
        }


        uint8_t word_term_size()
        {
            struct winsize size;

            auto result = ioctl(STDOUT_FILENO, TIOCGWINSZ, &size);

            if (result == -1)
            {
                set_last_error(("Could not read terminal information, " +
                                std::string(strerror(errno)) + ".").c_str());
            }

            stack_push_int(static_cast<int64_t>(size.ws_row));
            stack_push_int(static_cast<int64_t>(size.ws_col));

            return 0;
        }


        uint8_t word_term_key()
        {
            char next[2] = { 0 };
            ssize_t read_chars = 0;

            do
            {
                read_chars = read(STDIN_FILENO, next, 1);
            }
            while (read_chars == 0);

            stack_push_string(std::string(next).c_str());

            return 0;
        }


    #elif defined(_WIN64)


        void init_win_console()
        {
            SetConsoleCP(CP_UTF8);
            SetConsoleOutputCP(CP_UTF8);
        }


        uint8_t word_term_raw_mode()
        {
            auto flush_events =
                []()
                {
                    HANDLE std_in_handle = GetStdHandle(STD_INPUT_HANDLE);
                    DWORD dwRead;
                    INPUT_RECORD ir;
                    DWORD numberOfEvents = 0;
                    PeekConsoleInput(std_in_handle, nullptr, 0, &numberOfEvents);

                    if (numberOfEvents > 0)
                    {
                        while (true)
                        {
                            ReadConsoleInput(std_in_handle, &ir, 1, &dwRead);

                            if (   ir.EventType == KEY_EVENT
                                && ir.Event.KeyEvent.wVirtualKeyCode == VK_ESCAPE)
                            {
                                break;
                            }
                        }
                    }
                };

            static DWORD input_mode;
            static DWORD output_mode;
            static bool is_in_raw_mode = false;

            HANDLE std_out_handle = GetStdHandle(STD_OUTPUT_HANDLE);
            HANDLE std_in_handle = GetStdHandle(STD_INPUT_HANDLE);

            auto requested_on = interpreter->pop_as_bool();

            BOOL result = FALSE;

            if (requested_on && (!is_in_raw_mode))
            {
                result = GetConsoleMode(std_in_handle, &input_mode);
                throw_windows_error_if(!result, interpreter, "Get console input mode: ",
                                        GetLastError());

                result = GetConsoleMode(std_out_handle, &output_mode);
                throw_windows_error_if(!result, interpreter, "Get console input mode: ",
                                        GetLastError());


                DWORD new_input_mode = input_mode;
                DWORD new_output_mode = output_mode;

                new_input_mode &= ~(ENABLE_ECHO_INPUT | ENABLE_INSERT_MODE | ENABLE_LINE_INPUT |
                                    ENABLE_PROCESSED_INPUT);
                new_input_mode |= ENABLE_VIRTUAL_TERMINAL_INPUT;

                new_output_mode &= ~( ENABLE_INSERT_MODE | DISABLE_NEWLINE_AUTO_RETURN );
                new_output_mode |= ENABLE_PROCESSED_OUTPUT | ENABLE_VIRTUAL_TERMINAL_PROCESSING;


                result = SetConsoleMode(std_in_handle, new_input_mode);
                throw_windows_error_if(!result, interpreter, "Set console input mode: ",
                                        GetLastError());

                result = SetConsoleMode(std_out_handle, new_output_mode);
                throw_windows_error_if(!result, interpreter, "Set console output mode: ",
                                        GetLastError());

                flush_events();
                is_in_raw_mode = true;
            }
            else if ((!requested_on) && is_in_raw_mode)
            {
                result = SetConsoleMode(std_in_handle, input_mode);
                throw_windows_error_if(!result, interpreter, "Set console input mode: ",
                                        GetLastError());

                result = SetConsoleMode(std_out_handle, output_mode);
                throw_windows_error_if(!result, interpreter, "Set console output mode: ",
                                        GetLastError());

                flush_events();
                is_in_raw_mode = false;
            }
        }




        uint8_t word_term_size()
        {
            HANDLE std_out_handle = INVALID_HANDLE_VALUE;
            CONSOLE_SCREEN_BUFFER_INFO info = { 0 };

            std_out_handle = GetStdHandle(STD_OUTPUT_HANDLE);

            auto result = GetConsoleScreenBufferInfo(std_out_handle, &info);

            if (!result)
            {
                throw_windows_error(interpreter, "Could not get console information: ",
                                    GetLastError());
            }

            interpreter->push((int64_t)info.dwMaximumWindowSize.X);
            interpreter->push((int64_t)info.dwMaximumWindowSize.Y);
        }




        uint8_t word_term_key()
        {
            HANDLE std_in_handle = GetStdHandle(STD_INPUT_HANDLE);
            char buffer = 0;
            DWORD num_read = 0;

            auto result = ReadConsoleA(std_in_handle, &buffer, 1, &num_read, nullptr);

            throw_windows_error_if(!result, interpreter, "Read console error: ",
                                    GetLastError());

            interpreter->push(std::string(1, buffer));
        }


    #endif


    uint8_t word_term_flush()
    {
        std::cout << std::flush;

        return 0;
    }


    uint8_t word_term_read_line()
    {
        std::string line;
        std::getline(std::cin, line);

        stack_push_string(line.c_str());

        return 0;
    }


    uint8_t word_term_write()
    {
        Value value;

        auto pop_result = stack_pop(&value);

        if (pop_result)
        {
            return 1;
        }

        std::cout << value;

        return 0;
    }


    uint8_t word_term_is_printable()
    {
        Value string_value;


        auto pop_result = stack_pop(&string_value);

        if (pop_result)
        {
            return 1;
        }

        if (!string_value.is_string())
        {
            set_last_error("is-printable expected a string value.");

            return 1;
        }

        auto string = string_value.get_string();

        if (string.size() != 1)
        {
            set_last_error("is-printable expected single character.");

            return 1;
        }

        bool result =    (string[0] >= 32)
                      || (string[0] == '\n')
                      || (string[0] == '\t');

        stack_push_bool(result);

        return 0;
    }


}


namespace sorth::run_time::abi::words
{


    void register_terminal_words(const RuntimeWordRegistrar& registrar)
    {
        registrar("term.raw_mode", "word_term_raw_mode");
        registrar("term.size@", "word_term_size");
        registrar("term.key", "word_term_key");
        registrar("term.flush", "word_term_flush");
        registrar("term.readline", "word_term_read_line");
        registrar("term.!", "word_term_write");
        registrar("term.is_printable?", "word_term_is_printable");
    }


}

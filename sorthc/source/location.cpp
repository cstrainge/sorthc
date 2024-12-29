
#include "sorthc.h"



namespace sorth::source
{


    std::ostream& operator <<(std::ostream& stream, const Location& location)
    {
        stream << location.get_file() << ':'
               << location.get_line() << ':' << location.get_column();

        return stream;
    }


    std::strong_ordering operator <=>(const Location& lhs, const Location& rhs) noexcept
    {
        if (lhs.get_file() != rhs.get_file())
        {
            return lhs.get_file() <=> rhs.get_file();
        }

        if (lhs.get_line() != rhs.get_line())
        {
            return lhs.get_line() <=> rhs.get_line();
        }

        return lhs.get_column() <=> rhs.get_column();
    }


    Location::Location() noexcept
    : Location("<unknown>", 1, 1)
    {
    }


    Location::Location(const std::string& file) noexcept
    : Location(file, 1, 1)
    {
    }


    Location::Location(const std::string& file, size_t line, size_t column) noexcept
    : file(file),
      line(line),
      column(column)
    {
        std::replace(this->file.begin(), this->file.end(), '\\', '/');
    }


    const std::string& Location::get_file() const noexcept
    {
        return file;
    }


    size_t Location::get_line() const noexcept
    {
        return line;
    }


    size_t Location::get_column() const noexcept
    {
        return column;
    }


    void Location::increment_column() noexcept
    {
        ++column;
    }


    void Location::increment_line() noexcept
    {
        ++line;
        column = 1;
    }


}

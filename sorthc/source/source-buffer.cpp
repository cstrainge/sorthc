
#include "sorthc.h"



namespace sorthc::source
{


    SourceBuffer::SourceBuffer(const std::string& name, const std::string& text) noexcept
    : buffer(text),
      position(0),
      location(name)
    {
    }


    SourceBuffer::SourceBuffer(const std::filesystem::path& path)
    : SourceBuffer(path.string(), load_source(path))
    {
    }


    SourceBuffer::operator bool() const
    {
        return position < buffer.size();
    }


    char SourceBuffer::peek() const noexcept
    {
        return *this ? buffer[position] : ' ';
    }


    char SourceBuffer::next() noexcept
    {
        auto next = peek();

        if (*this)
        {
            increment_location(next);
        }

        return next;
    }


    Location SourceBuffer::get_location() const noexcept
    {
        return location;
    }


    void SourceBuffer::increment_location(char next) noexcept
    {
        ++position;

        if (next == '\n')
        {
            location.increment_line();
        }
        else
        {
            location.increment_column();
        }
    }


    std::string SourceBuffer::load_source(const std::filesystem::path& path)
    {
        std::ifstream source_file(path.c_str());

        auto begin = std::istreambuf_iterator<char>(source_file);
        auto end = std::istreambuf_iterator<char>();

        return std::string(begin, end);
    }


}

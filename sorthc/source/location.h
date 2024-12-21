
#pragma once



namespace sorthc::source
{


    class Location
    {
        private:
            std::string file;
            size_t line;
            size_t column;

        public:
            Location() noexcept;
            Location(const std::string& file) noexcept;
            Location(const std::string& file, size_t line, size_t column) noexcept;
            Location(const Location& location) noexcept = default;
            Location(Location&& location) noexcept = default;
            ~Location() noexcept = default;

        public:
            Location& operator =(const Location& location) noexcept = default;
            Location& operator =(Location&& location) noexcept = default;

        public:
            const std::string& get_file() const noexcept;
            size_t get_line() const noexcept;
            size_t get_column() const noexcept;

        protected:
            void increment_column() noexcept;
            void increment_line() noexcept;

        private:
            friend class SourceBuffer;
    };


    // Get a location for the current position in the C++ source code.
    #define LOCATION_HERE sorthc::source::Location(__FILE__, __LINE__, 1)


    std::ostream& operator <<(std::ostream& stream, const Location& location);


    std::strong_ordering operator <=>(const Location& lhs, const Location& rhs) noexcept;


    inline bool operator ==(const Location& lhs, const Location& rhs) noexcept
    {
        return (lhs <=> rhs) == std::strong_ordering::equal;
    }


    inline bool operator !=(const Location& lhs, const Location& rhs) noexcept
    {
        return (lhs <=> rhs) != std::strong_ordering::equal;
    }


}

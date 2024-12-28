
#pragma once



namespace sorth::source
{


    class SourceBuffer
    {
        private:
            std::string buffer;
            size_t position;
            Location location;

        public:
            SourceBuffer(const std::string& name, const std::string& text) noexcept;
            SourceBuffer(const std::filesystem::path& path);

        public:
            operator bool() const;

        public:
            char peek() const noexcept;
            char next() noexcept;

            Location get_location() const noexcept;

        private:
            void increment_location(char next) noexcept;
            std::string load_source(const std::filesystem::path& path);
    };


}

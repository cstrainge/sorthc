
#pragma once



namespace sorthc
{


    class Token
    {
        public:
            enum class Type
            {
                none,
                word,
                string,
                integer,
                floating
            };

        private:
            using ValueType = std::variant<std::string, int64_t, double>;

        private:
            Location location;
            Type type;
            ValueType value;

        public:
            Token() noexcept;
            Token(const Location& location, Type type, const std::string& value) noexcept;
            Token(const Location& location, int64_t value) noexcept;
            Token(const Location& location, double value) noexcept;
            Token(const Token& token) noexcept = default;
            Token(Token&& token) noexcept = default;
            ~Token() noexcept = default;

        public:
            Token& operator=(const Token& token) noexcept = default;
            Token& operator=(Token&& token) noexcept = default;

            operator bool() const noexcept;

        public:
            const Location& get_location() const noexcept;
            Type get_type() const noexcept;

            std::string get_text() const noexcept;
            int64_t get_integer() const noexcept;
            double get_number() const noexcept;

            std::string get_as_word() const;
    };


    using TokenList = std::vector<Token>;


    std::ostream& operator <<(std::ostream& stream, const Token& token);


    std::strong_ordering operator <=>(const Token& lhs, const Token& rhs) noexcept;


    inline bool operator ==(const Token& lhs, const Token& rhs) noexcept
    {
        return (lhs <=> rhs) == std::strong_ordering::equal;
    }


    inline bool operator !=(const Token& lhs, const Token& rhs) noexcept
    {
        return (lhs <=> rhs) != std::strong_ordering::equal;
    }


}


#include "sorthc.h"



namespace sorthc::source
{


    std::ostream& operator <<(std::ostream& stream, const Token& token)
    {
        stream << token.get_location() << ": ";

        switch (token.get_type())
        {
            case Token::Type::none:
                stream << "<none>";
                break;

            case Token::Type::word:
                stream << "<word>: " << token.get_text();
                break;

            case Token::Type::string:
                stream << "<string>: " << token.get_text();
                break;

            case Token::Type::integer:
                stream << "<integer>: " << token.get_integer();
                break;

            case Token::Type::floating:
                stream << "<float>: " << token.get_number();
                break;
        }

        return stream;
    }


    std::strong_ordering operator <=>(const Token& lhs, const Token& rhs) noexcept
    {
        // Location is ignored for this test.

        if (lhs.get_type() != rhs.get_type())
        {
            return lhs.get_type() <=> rhs.get_type();
        }

        std::strong_ordering result = std::strong_ordering::equal;

        switch (lhs.get_type())
        {
            case Token::Type::none:
                break;

            case Token::Type::word:
            case Token::Type::string:
                result = lhs.get_text() <=> rhs.get_text();
                break;

            case Token::Type::integer:
                result = lhs.get_integer() <=> rhs.get_integer();
                break;

            case Token::Type::floating:
                {
                    double left = lhs.get_number();
                    double right = rhs.get_number();

                    if (left < right)
                    {
                        result = std::strong_ordering::less;
                    }
                    else if (left > right)
                    {
                        result = std::strong_ordering::greater;
                    }
                }
                break;
        }

        return result;
    }


    Token::Token() noexcept
    : location(),
      type(Type::none),
      value()
    {
    }


    Token::Token(const Location& location, Type type, const std::string& value) noexcept
    : location(location),
      type(type),
      value(value)
    {
        assert(type == Type::word || type == Type::string);
    }


    Token::Token(const Location& location, int64_t value) noexcept
    : location(location),
      type(Type::integer),
      value(value)
    {
    }


    Token::Token(const Location& location, double value) noexcept
    : location(location),
      type(Type::floating),
      value(value)
    {
    }


    Token::operator bool() const noexcept
    {
        return type != Type::none;
    }


    const Location& Token::get_location() const noexcept
    {
        return location;
    }


    Token::Type Token::get_type() const noexcept
    {
        return type;
    }


    std::string Token::get_text() const noexcept
    {
        if (type == Type::word || type == Type::string)
        {
            return std::get<std::string>(value);
        }

        throw_error(location, "Token is not a word or string.");
    }


    int64_t Token::get_integer() const noexcept
    {
        if (   (type == Type::none)
            || (type == Type::word)
            || (type == Type::string))
        {
            throw_error(location, "Token is not a numeric value.");
        }

        if (type == Type::floating)
        {
            return static_cast<int64_t>(std::get<double>(value));
        }

        return std::get<int64_t>(value);
    }


    double Token::get_number() const noexcept
    {
        if (   (type == Type::none)
            || (type == Type::word)
            || (type == Type::string))
        {
            throw_error(location, "Token is not a numeric value.");
        }

        if (type == Type::integer)
        {
            return static_cast<double>(std::get<int64_t>(value));
        }

        return std::get<double>(value);
    }


    std::string Token::get_as_word() const
    {
        if (   (type == Type::none)
            || (type == Type::string))
        {
            throw_error(location, "Token is not a word type.");
        }

        if (type == Type::integer)
        {
            return std::to_string(std::get<int64_t>(value));
        }

        if (type == Type::floating)
        {
            return std::to_string(std::get<double>(value));
        }

        return std::get<std::string>(value);
    }


}

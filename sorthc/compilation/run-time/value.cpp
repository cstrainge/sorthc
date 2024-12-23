
#include "sorthc.h"



namespace sorthc::compilation::run_time
{


    std::ostream& operator <<(std::ostream& stream, const Value& value) noexcept
    {
        if (value.is_none())
        {
            stream << "none";
        }
        else if (value.is_int())
        {
            stream << value.get_int();
        }
        else if (value.is_double())
        {
            stream << value.get_double();
        }
        else if (value.is_bool())
        {
            stream << (value.get_bool() ? "true" : "false");
        }
        else if (value.is_string())
        {
            stream << value.get_string();
        }
        else if (value.is_byte_code())
        {
            stream << "<byte code>";
        }

        return stream;
    }


    // Return a string value, but convert the value to a string if it is not a string.  Also enclose
    // the string in quotes, and will escape any characters that need to be escaped.
    std::string stringify(const Value& value) noexcept
    {
        std::stringstream stream;

        stream << value;

        return stringify(stream.str());
    }

    // Enclose a string in quotes and escape any characters that need to be escaped.
    std::string stringify(const std::string& value) noexcept
    {
        std::stringstream output;

        output << "\"";

        for (size_t i = 0; i < value.size(); ++i)
        {
            char next = value[i];

            switch (next)
            {
                case '\r': output << "\\r";  break;
                case '\n': output << "\\n";  break;
                case '\t': output << "\\n";  break;
                case '\"': output << "\\\""; break;

                default:
                    if (!isprint(next))
                    {
                        output << "\\0" << (int)next;
                    }
                    else
                    {
                        output << next;
                    }
                    break;
            }
        }

        output << "\"";

        return output.str();
    }


    Value::Value()
    : value(None())
    {
    }


    Value::Value(size_t value)
    : value(static_cast<int64_t>(value))
    {
    }


    Value::Value(int64_t value)
    : value(value)
    {
    }


    Value::Value(double value)
    : value(value)
    {
    }


    Value::Value(bool value)
    : value(value)
    {
    }


    Value::Value(const char* value)
    : value(std::string(value))
    {
    }


    Value::Value(const std::string& value)
    : value(value)
    {
    }


    Value::Value(const byte_code::ByteCode& value)
    : value(value)
    {
    }


    Value::Value(const source::Token& token)
    : value(None())
    {
        if (token.get_type() == source::Token::Type::string)
        {
            value = token.get_text();
        }
        else
        {
            value = token.get_as_word();
        }
    }


    bool Value::is_none() const noexcept
    {
        return std::holds_alternative<None>(value);
    }


    bool Value::is_numeric() const noexcept
    {
        return is_int() || is_double() || is_bool();
    }


    bool Value::is_int() const noexcept
    {
        return std::holds_alternative<int64_t>(value);
    }


    bool Value::is_double() const noexcept
    {
        return std::holds_alternative<double>(value);
    }


    bool Value::is_bool() const noexcept
    {
        return std::holds_alternative<bool>(value);
    }


    bool Value::is_string() const noexcept
    {
        return std::holds_alternative<std::string>(value);
    }


    bool Value::is_byte_code() const noexcept
    {
        return std::holds_alternative<byte_code::ByteCode>(value);
    }


    int64_t Value::get_int() const
    {
        if (is_double())
        {
            return static_cast<int64_t>(std::get<double>(value));
        }
        else if (is_bool())
        {
            return std::get<bool>(value) ? 1 : 0;
        }

        assert(is_int());

        return std::get<int64_t>(value);
    }


    double Value::get_double() const
    {
        if (is_int())
        {
            return static_cast<double>(std::get<int64_t>(value));
        }
        else if (is_bool())
        {
            return std::get<bool>(value) ? 1.0 : 0.0;
        }

        assert(is_double());

        return std::get<double>(value);
    }


    bool Value::get_bool() const
    {
        if (is_int())
        {
            return std::get<int64_t>(value) != 0;
        }
        else if (is_double())
        {
            return std::get<double>(value) != 0.0;
        }

        assert(is_bool());

        return std::get<bool>(value);
    }


    const std::string& Value::get_string() const
    {
        assert(is_string());

        return std::get<std::string>(value);
    }


    const byte_code::ByteCode& Value::get_byte_code() const
    {
        assert(is_byte_code());

        return std::get<byte_code::ByteCode>(value);
    }


    int64_t Value::get_int(const CompilerRuntime& runtime) const
    {
        if (is_double())
        {
            return static_cast<int64_t>(std::get<double>(value));
        }
        else if (is_bool())
        {
            return std::get<bool>(value) ? 1 : 0;
        }

        throw_error_if(!is_int(), runtime, "Value is not an integer.");

        return std::get<int64_t>(value);
    }


    double Value::get_double(const CompilerRuntime& runtime) const
    {
        if (is_int())
        {
            return static_cast<double>(std::get<int64_t>(value));
        }
        else if (is_bool())
        {
            return std::get<bool>(value) ? 1.0 : 0.0;
        }

        throw_error_if(!is_double(), runtime, "Value is not a double.");

        return std::get<double>(value);
    }


    bool Value::get_bool(const CompilerRuntime& runtime) const
    {
        if (is_int())
        {
            return std::get<int64_t>(value) != 0;
        }
        else if (is_double())
        {
            return std::get<double>(value) != 0.0;
        }

        throw_error_if(!is_bool(), runtime, "Value is not a boolean.");

        return std::get<bool>(value);
    }


    const std::string& Value::get_string(const CompilerRuntime& runtime) const
    {
        throw_error_if(!is_string(), runtime, "Value is not a string.");

        return std::get<std::string>(value);
    }


    const byte_code::ByteCode& Value::get_byte_code(const CompilerRuntime& runtime) const
    {
        throw_error_if(!is_byte_code(), runtime, "Value is not byte code.");

        return std::get<byte_code::ByteCode>(value);
    }


}

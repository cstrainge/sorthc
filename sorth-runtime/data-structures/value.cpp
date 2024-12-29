
#include "sorth-runtime.h"



namespace sorth::run_time::data_structures
{


    std::string stringify(const Value& value) noexcept
    {
        return stringify(value.get_string_with_conversion());
    }


    std::string stringify(const std::string& value) noexcept
    {
        std::string output = "\"";

        for (size_t i = 0; i < value.size(); ++i)
        {
            char next = value[i];

            switch (next)
            {
                case '\r': output += "\\r";  break;
                case '\n': output += "\\n";  break;
                case '\t': output += "\\n";  break;
                case '\"': output += "\\\""; break;

                default:
                    if (!isprint(next))
                    {
                        output += "\\0" + std::to_string((int)next);
                    }
                    else
                    {
                        output += next;
                    }
                    break;
            }
        }

        output += "\"";

        return output;
    }


    std::ostream& operator <<(std::ostream& stream, const Value& value) noexcept
    {
        if (std::holds_alternative<None>(value.value))
        {
            stream << "none";
        }
        else if (std::holds_alternative<int64_t>(value.value))
        {
            stream << std::get<int64_t>(value.value);
        }
        else if (std::holds_alternative<double>(value.value))
        {
            stream << std::get<double>(value.value);
        }
        else if (std::holds_alternative<bool>(value.value))
        {
            stream << (std::get<bool>(value.value) ? "true" : "false");
        }
        else if (std::holds_alternative<std::string>(value.value))
        {
            stream << std::get<std::string>(value.value);
        }
        else if (std::holds_alternative<StructurePtr>(value.value))
        {
            stream << std::get<StructurePtr>(value.value);
        }
        else if (std::holds_alternative<ArrayPtr>(value.value))
        {
            stream << std::get<ArrayPtr>(value.value);
        }
        else if (std::holds_alternative<HashTablePtr>(value.value))
        {
            stream << std::get<HashTablePtr>(value.value);
        }
        else if (std::holds_alternative<ByteBufferPtr>(value.value))
        {
            stream << std::get<ByteBufferPtr>(value.value);
        }
        else
        {
            stream << "<unknown-value-type>";
        }

        return stream;
    }


    std::strong_ordering operator <=>(const Value& lhs, const Value& rhs) noexcept
    {
        if (lhs.value.index() != rhs.value.index())
        {
            return lhs.value.index() <=> rhs.value.index();
        }

        if (std::holds_alternative<None>(lhs.value))
        {
            return std::strong_ordering::equal;
        }

        if (std::holds_alternative<int64_t>(lhs.value))
        {
            return std::get<int64_t>(lhs.value) <=> std::get<int64_t>(rhs.value);
        }

        if (std::holds_alternative<double>(lhs.value))
        {
            auto lhs_value = std::get<double>(lhs.value);
            auto rhs_value = std::get<double>(rhs.value);

            if (lhs_value > rhs_value)
            {
                return std::strong_ordering::greater;
            }
            else if (lhs_value < rhs_value)
            {
                return std::strong_ordering::less;
            }

            return std::strong_ordering::equal;
        }

        if (std::holds_alternative<bool>(lhs.value))
        {
            return std::get<bool>(lhs.value) <=> std::get<bool>(rhs.value);
        }

        if (std::holds_alternative<std::string>(lhs.value))
        {
            return std::get<std::string>(lhs.value) <=> std::get<std::string>(rhs.value);
        }

        if (std::holds_alternative<StructurePtr>(lhs.value))
        {
            return std::get<StructurePtr>(lhs.value) <=> std::get<StructurePtr>(rhs.value);
        }

        if (std::holds_alternative<ArrayPtr>(lhs.value))
        {
            return std::get<ArrayPtr>(lhs.value) <=> std::get<ArrayPtr>(rhs.value);
        }

        if (std::holds_alternative<HashTablePtr>(lhs.value))
        {
            return std::get<HashTablePtr>(lhs.value) <=> std::get<HashTablePtr>(rhs.value);
        }

        if (std::holds_alternative<ByteBufferPtr>(lhs.value))
        {
            return std::get<ByteBufferPtr>(lhs.value) <=> std::get<ByteBufferPtr>(rhs.value);
        }

        return std::strong_ordering::equal;
    }


    thread_local size_t Value::value_format_indent;


    Value::Value() noexcept
    : value(None{})
    {
    }


    Value::Value(int64_t new_value) noexcept
    : value(new_value)
    {
    }


    Value::Value(double new_value) noexcept
    : value(new_value)
    {
    }


    Value::Value(bool new_value) noexcept
    : value(new_value)
    {
    }


    Value::Value(const char* new_value) noexcept
    : value(std::string(new_value))
    {
    }


    Value::Value(const std::string& new_value) noexcept
    : value(new_value)
    {
    }


    Value::Value(const StructurePtr& new_value) noexcept
    : value(new_value)
    {
    }


    Value::Value(const ArrayPtr& new_value) noexcept
    : value(new_value)
    {
    }


    Value::Value(const HashTablePtr& new_value) noexcept
    : value(new_value)
    {
    }


    Value::Value(const ByteBufferPtr& new_value) noexcept
    : value(new_value)
    {
    }


    Value& Value::operator =(const None& new_value) noexcept
    {
        value = new_value;

        return *this;
    }


    Value& Value::operator =(int64_t new_value) noexcept
    {
        value = new_value;

        return *this;
    }


    Value& Value::operator =(double new_value) noexcept
    {
        value = new_value;

        return *this;
    }


    Value& Value::operator =(bool new_value) noexcept
    {
        value = new_value;

        return *this;
    }


    Value& Value::operator =(const char* new_value) noexcept
    {
        value = std::string(new_value);

        return *this;
    }


    Value& Value::operator =(const std::string& new_value) noexcept
    {
        value = new_value;

        return *this;
    }


    Value& Value::operator =(const ArrayPtr& new_value) noexcept
    {
        value = new_value;

        return *this;
    }


    Value Value::deep_copy() const noexcept
    {
        if (is_structure())
        {
            return std::get<StructurePtr>(value)->deep_copy();
        }
        else if (is_array())
        {
            return std::get<ArrayPtr>(value)->deep_copy();
        }
        else if (is_hash_table())
        {
            return std::get<HashTablePtr>(value)->deep_copy();
        }
        else if (is_byte_buffer())
        {
            return std::get<ByteBufferPtr>(value)->deep_copy();
        }

        return *this;
    }


    bool Value::is_none() const noexcept
    {
        return std::holds_alternative<None>(value);
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


    bool Value::is_structure() const noexcept
    {
        return std::holds_alternative<StructurePtr>(value);
    }


    bool Value::is_array() const noexcept
    {
        return std::holds_alternative<ArrayPtr>(value);
    }


    bool Value::is_hash_table() const noexcept
    {
        return std::holds_alternative<HashTablePtr>(value);
    }


    bool Value::is_byte_buffer() const noexcept
    {
        return std::holds_alternative<ByteBufferPtr>(value);
    }


    bool Value::is_numeric() const noexcept
    {
        return is_int() || is_double() || is_bool();
    }


    bool Value::either_is_string(const Value& a, const Value& b) noexcept
    {
        return a.is_string() || b.is_string();
    }


    bool Value::either_is_numeric(const Value& a, const Value& b) noexcept
    {
        return a.is_numeric() || b.is_numeric();
    }


    bool Value::either_is_integer(const Value& a, const Value& b) noexcept
    {
        return a.is_int() || b.is_int();
    }


    bool Value::either_is_float(const Value& a, const Value& b) noexcept
    {
        return a.is_double() || b.is_double();
    }


    int64_t Value::get_int() const
    {
        if (!is_numeric())
        {
            throw std::runtime_error("Value is not numeric.");
        }

        if (is_double())
        {
            return static_cast<int64_t>(std::get<double>(value));
        }
        else if (is_bool())
        {
            return std::get<bool>(value) ? 1 : 0;
        }

        return std::get<int64_t>(value);
    }


    double Value::get_double() const
    {
        if (!is_numeric())
        {
            throw std::runtime_error("Value is not numeric.");
        }

        if (is_int())
        {
            return static_cast<double>(std::get<int64_t>(value));
        }
        else if (is_bool())
        {
            return std::get<bool>(value) ? 1.0 : 0.0;
        }

        return std::get<double>(value);
    }


    bool Value::get_bool() const
    {
        if (!is_numeric())
        {
            throw std::runtime_error("Value is not numeric.");
        }

        if (is_int())
        {
            return std::get<int64_t>(value) != 0;
        }
        else if (is_double())
        {
            return std::get<double>(value) != 0.0;
        }

        return std::get<bool>(value);
    }


    const std::string& Value::get_string() const
    {
        if (!is_string())
        {
            throw std::runtime_error("Value is not a string.");
        }

        return std::get<std::string>(value);
    }


    std::string Value::get_string_with_conversion() const noexcept
    {
        if (is_string())
        {
            return get_string();
        }

        std::stringstream stream;

        stream << *this;

        return stream.str();
    }


    StructurePtr Value::get_structure() const
    {
        if (!is_structure())
        {
            throw std::runtime_error("Value is not a structure.");
        }

        return std::get<StructurePtr>(value);
    }


    ArrayPtr Value::get_array() const
    {
        if (!is_array())
        {
            throw std::runtime_error("Value is not an array.");
        }

        return std::get<ArrayPtr>(value);
    }


    HashTablePtr Value::get_hash_table() const
    {
        if (!is_hash_table())
        {
            throw std::runtime_error("Value is not a hash table.");
        }

        return std::get<HashTablePtr>(value);
    }


    ByteBufferPtr Value::get_byte_buffer() const
    {
        if (!is_byte_buffer())
        {
            throw std::runtime_error("Value is not a byte buffer.");
        }

        return std::get<ByteBufferPtr>(value);
    }


    size_t Value::hash() const noexcept
    {
        if (is_none())
        {
            return std::hash<int>()(0);
        }

        if (is_int())
        {
            return std::hash<int64_t>()(std::get<int64_t>(value));
        }

        if (is_double())
        {
            return std::hash<double>()(std::get<double>(value));
        }

        if (is_bool())
        {
            return std::hash<bool>()(std::get<bool>(value));
        }

        if (is_string())
        {
            return std::hash<std::string>()(std::get<std::string>(value));
        }

        if (is_structure())
        {
            return std::get<StructurePtr>(value)->hash();
        }

        if (is_array())
        {
            return std::get<ArrayPtr>(value)->hash();
        }

        if (is_hash_table())
        {
            return std::get<HashTablePtr>(value)->hash();
        }

        if (is_byte_buffer())
        {
            return std::get<ByteBufferPtr>(value)->hash();
        }

        return 0;
    }


    void Value::hash_combine(size_t& seed, size_t value) noexcept
    {
        seed ^= seed ^ (value + 0x9e3779b9 + (seed << 6) + (seed >> 2));
    }


}


#include "sorth-runtime.h"



namespace sorth::run_time::data_structures
{


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


    bool Value::is_numeric() const noexcept
    {
        return is_int() || is_double() || is_bool();
    }


    int64_t Value::get_int() const
    {
        assert(is_numeric());

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
        assert(is_numeric());

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
        assert(is_numeric());

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
        assert(is_string());

        return std::get<std::string>(value);
    }


}

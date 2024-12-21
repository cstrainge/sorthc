
#include "sorthc.h"



namespace sorthc::compilation::run_time
{


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


}


#pragma once



namespace sorth::run_time::data_structures
{


    struct None
    {
    };


    class Value
    {
        private:
            using ValueType = std::variant<None, int64_t, double, bool, std::string>;

        private:
            ValueType value;

        public:
            Value() noexcept;
            Value(int64_t new_value) noexcept;
            Value(double new_value) noexcept;
            Value(bool new_value) noexcept;
            Value(const char* new_value) noexcept;
            Value(const std::string& new_value) noexcept;
            Value(const Value& other) noexcept = default;
            Value(Value&& other) noexcept = default;
            ~Value() noexcept = default;

        public:
            Value& operator =(const None& new_value) noexcept;
            Value& operator =(int64_t new_value) noexcept;
            Value& operator =(double new_value) noexcept;
            Value& operator =(bool new_value) noexcept;
            Value& operator =(const char* new_value) noexcept;
            Value& operator =(const std::string& new_value) noexcept;
            Value& operator =(const Value& other) noexcept = default;
            Value& operator =(Value&& other) noexcept = default;

        public:
            bool is_none() const noexcept;
            bool is_int() const noexcept;
            bool is_double() const noexcept;
            bool is_bool() const noexcept;
            bool is_string() const noexcept;

            bool is_numeric() const noexcept;

        public:
            int64_t get_int() const;
            double get_double() const;
            bool get_bool() const;
            const std::string& get_string() const;
    };


}

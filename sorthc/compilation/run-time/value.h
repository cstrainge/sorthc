
#pragma once



namespace sorthc::compilation::run_time
{


    struct None
    {
    };


    class CompilerRuntime;


    class Value
    {
        private:
            using ValueType = std::variant<None, int64_t, double, bool, std::string>;

        private:
            ValueType value;

        public:
            Value();
            Value(int64_t value);
            Value(size_t value);
            Value(double value);
            Value(bool value);
            Value(const char* value);
            Value(const std::string& value);
            Value(const Value& other) noexcept = default;
            Value(Value&& other) noexcept = default;
            ~Value() noexcept = default;

        public:
            Value& operator =(const Value& other) noexcept = default;
            Value& operator =(Value&& other) noexcept = default;

        public:
            bool is_none() const noexcept;
            bool is_numeric() const noexcept;
            bool is_int() const noexcept;
            bool is_double() const noexcept;
            bool is_bool() const noexcept;
            bool is_string() const noexcept;

        public:
            int64_t get_int() const;
            double get_double() const;
            bool get_bool() const;
            const std::string& get_string() const;

            int64_t get_int(const CompilerRuntime& runtime) const;
            double get_double(const CompilerRuntime& runtime) const;
            bool get_bool(const CompilerRuntime& runtime) const;
            const std::string& get_string(const CompilerRuntime& runtime) const;
    };


}

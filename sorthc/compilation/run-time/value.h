
#pragma once



namespace sorthc::compilation::byte_code
{


    class Instruction;
    using ByteCode = std::vector<Instruction>;


}


namespace sorthc::compilation::run_time
{


    class CompilerRuntime;


    // Represent a nothing value in the run-time.
    struct None
    {
    };


    // Represents a value that can be stored in the run-time's data stack or in a variable.  It's a
    // bit limited vs the full Strange Forth value, but it's enough to get the job done.
    class Value
    {
        private:
            using ValueType = std::variant<None,
                                           int64_t,
                                           double,
                                           bool,
                                           std::string,
                                           byte_code::ByteCode>;

        private:
            // The actual data storage of the Value type.
            ValueType value;

        public:
            Value();
            Value(int64_t value);
            Value(size_t value);
            Value(double value);
            Value(bool value);
            Value(const char* value);
            Value(const std::string& value);
            Value(const byte_code::ByteCode& value);
            Value(const source::Token& token);
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
            bool is_byte_code() const noexcept;

        public:
            int64_t get_int() const;
            double get_double() const;
            bool get_bool() const;
            const std::string& get_string() const;
            const byte_code::ByteCode& get_byte_code() const;

            int64_t get_int(const CompilerRuntime& runtime) const;
            double get_double(const CompilerRuntime& runtime) const;
            bool get_bool(const CompilerRuntime& runtime) const;
            const std::string& get_string(const CompilerRuntime& runtime) const;
            const byte_code::ByteCode& get_byte_code(const CompilerRuntime& runtime) const;
    };


    std::ostream& operator <<(std::ostream& stream, const Value& value) noexcept;


    // Return a string value, but convert the value to a string if it is not a string.  Also enclose
    // the string in quotes, and will escape any characters that need to be escaped.
    std::string stringify(const Value& value) noexcept;

    // Enclose a string in quotes and escape any characters that need to be escaped.
    std::string stringify(const std::string& value) noexcept;


}

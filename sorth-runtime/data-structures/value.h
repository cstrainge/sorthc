
#pragma once



namespace sorth::run_time::data_structures
{


    struct None
    {
    };


    class Structure;
    using StructurePtr = std::shared_ptr<Structure>;


    class Array;
    using ArrayPtr = std::shared_ptr<Array>;


    class HashTable;
    using HashTablePtr = std::shared_ptr<HashTable>;


    class ByteBuffer;
    using ByteBufferPtr = std::shared_ptr<ByteBuffer>;


    class Value
    {
        private:
            using ValueType = std::variant<None,
                                           int64_t,
                                           double,
                                           bool,
                                           std::string,
                                           StructurePtr,
                                           ArrayPtr,
                                           HashTablePtr,
                                           ByteBufferPtr>;

        public:
            static thread_local size_t value_format_indent;

        private:
            ValueType value;

        public:
            Value() noexcept;
            Value(int64_t new_value) noexcept;
            Value(double new_value) noexcept;
            Value(bool new_value) noexcept;
            Value(const char* new_value) noexcept;
            Value(const std::string& new_value) noexcept;
            Value(const StructurePtr& new_value) noexcept;
            Value(const ArrayPtr& new_value) noexcept;
            Value(const HashTablePtr& new_value) noexcept;
            Value(const ByteBufferPtr& new_value) noexcept;
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
            Value& operator =(const StructurePtr& new_value) noexcept;
            Value& operator =(const ArrayPtr& new_value) noexcept;
            Value& operator =(const HashTablePtr& new_value) noexcept;
            Value& operator =(const ByteBufferPtr& new_value) noexcept;
            Value& operator =(const Value& other) noexcept = default;
            Value& operator =(Value&& other) noexcept = default;

        public:
            Value deep_copy() const noexcept;

        public:
            bool is_none() const noexcept;
            bool is_int() const noexcept;
            bool is_double() const noexcept;
            bool is_bool() const noexcept;
            bool is_string() const noexcept;
            bool is_structure() const noexcept;
            bool is_array() const noexcept;
            bool is_hash_table() const noexcept;
            bool is_byte_buffer() const noexcept;

            bool is_numeric() const noexcept;

        public:
            static bool either_is_string(const Value& a, const Value& b) noexcept;
            static bool either_is_numeric(const Value& a, const Value& b) noexcept;
            static bool either_is_integer(const Value& a, const Value& b) noexcept;
            static bool either_is_float(const Value& a, const Value& b) noexcept;

        public:
            int64_t get_int() const;
            double get_double() const;
            bool get_bool() const;
            const std::string& get_string() const;
            std::string get_string_with_conversion() const noexcept;
            StructurePtr get_structure() const;
            ArrayPtr get_array() const;
            HashTablePtr get_hash_table() const;
            ByteBufferPtr get_byte_buffer() const;

        public:
            size_t hash() const noexcept;
            static void hash_combine(size_t& seed, size_t value) noexcept;

        private:
            friend std::ostream& operator <<(std::ostream& stream, const Value& value) noexcept;
            friend std::strong_ordering operator <=>(const Value& lhs, const Value& rhs) noexcept;
    };

    // Return a string value, but convert the value to a string if it is not a string.  Also enclose
    // the string in quotes, and will escape any characters that need to be escaped.
    std::string stringify(const Value& value) noexcept;

    // Enclose a string in quotes and escape any characters that need to be escaped.
    std::string stringify(const std::string& value) noexcept;


    std::ostream& operator <<(std::ostream& stream, const Value& value) noexcept;


    std::strong_ordering operator <=>(const Value& lhs, const Value& rhs) noexcept;


    inline bool operator ==(const Value& lhs, const Value& rhs) noexcept
    {
        return (lhs <=> rhs) == std::strong_ordering::equal;
    }

    inline bool operator !=(const Value& lhs, const Value& rhs) noexcept
    {
        return (lhs <=> rhs) != std::strong_ordering::equal;
    }


}


namespace std
{


    template<>
    struct hash<sorth::run_time::data_structures::Value>
    {
        inline size_t operator()(const sorth::run_time::data_structures::Value& key) const
        {
            return key.hash();
        }
    };


}


#pragma once



namespace sorth::compilation::run_time
{


    class CompilerRuntime;


    class Array
    {
        private:
            std::vector<Value> items;

        public:
            Array(size_t size);

        public:
            size_t size() const;
            Value& operator [](size_t index);
            void resize(size_t new_size);

            void insert(size_t index, const Value& value);
            void remove(size_t index);

        public:
            void push_front(const Value& value);
            void push_back(const Value& value);
            Value pop_front(CompilerRuntime& runtime);
            Value pop_back(CompilerRuntime& runtime);

        private:
            friend std::ostream& operator <<(std::ostream& stream, const ArrayPtr& array);
            friend std::strong_ordering operator <=>(const Array& lhs, const Array& rhs);
    };


    using ArrayPtr = std::shared_ptr<Array>;


    std::ostream& operator <<(std::ostream& stream, const ArrayPtr& array);


    std::strong_ordering operator <=>(const Array& lhs, const Array& rhs);

    std::strong_ordering operator <=>(const ArrayPtr& lhs, const ArrayPtr& rhs);


    inline bool operator ==(const Array& lhs, const Array& rhs)
    {
        return (lhs <=> rhs) == std::strong_ordering::equal;
    }

    inline bool operator !=(const Array& lhs, const Array& rhs)
    {
        return (lhs <=> rhs) != std::strong_ordering::equal;
    }

    inline bool operator ==(const ArrayPtr& lhs, const ArrayPtr& rhs)
    {
        return *lhs == *rhs;
    }

    inline bool operator !=(const ArrayPtr& lhs, const ArrayPtr& rhs)
    {
        return *lhs != *rhs;
    }


}

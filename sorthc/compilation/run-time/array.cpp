
#include "sorthc.h"



namespace sorth::compilation::run_time
{


    std::ostream& operator <<(std::ostream& stream, const ArrayPtr& array)
    {
        stream << "[ ";

        for (size_t i = 0; i < array->items.size(); ++i)
        {
            stream << array->items[i];

            if (i < (array->items.size() - 1))
            {
                stream << " , ";
            }
        }

        stream << " ]";

        return stream;
    }


    std::strong_ordering operator <=>(const Array& lhs, const Array& rhs)
    {
        return lhs.items <=> rhs.items;
    }


    std::strong_ordering operator <=>(const ArrayPtr& lhs, const ArrayPtr& rhs)
    {
        return *lhs <=> *rhs;
    }


    Array::Array(size_t size)
    {
        items.resize(size);
    }


    size_t Array::size() const
    {
        return items.size();
    }


    Value& Array::operator [](size_t index)
    {
        return items[index];
    }


    void Array::resize(size_t new_size)
    {
        items.resize((size_t)new_size);
    }


    void Array::insert(size_t index, const Value& value)
    {
        items.insert(std::next(items.begin(), index), value);
    }


    void Array::remove(size_t index)
    {
        items.erase(std::next(items.begin(), index));
    }


    void Array::push_front(const Value& value)
    {
        items.insert(items.begin(), value);
    }


    void Array::push_back(const Value& value)
    {
        items.push_back(value);
    }


    Value Array::pop_front(CompilerRuntime& runtime)
    {
        if (items.empty())
        {
            throw_error(runtime, "Popping from an empty array.");
        }

        Value value = items[0];

        items.erase(items.begin());

        return value;
    }


    Value Array::pop_back(CompilerRuntime& runtime)
    {
        if (items.empty())
        {
            throw_error(runtime, "Popping from an empty array.");
        }

        Value value = items[0];

        items.pop_back();

        return value;
    }


}

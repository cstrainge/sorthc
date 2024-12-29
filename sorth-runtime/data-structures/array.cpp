
#include "sorth-runtime.h"



namespace sorth::run_time::data_structures
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

    Value Array::pop_front()
    {
        if (items.empty())
        {
            throw std::runtime_error("Popping from an empty array.");
        }

        Value value = items[0];

        items.erase(items.begin());

        return value;
    }

    Value Array::pop_back()
    {
        if (items.empty())
        {
            throw std::runtime_error("Popping from an empty array.");
        }

        Value value = items[0];

        items.pop_back();

        return value;
    }


    Value Array::deep_copy() const noexcept
    {
        ArrayPtr result = std::make_shared<Array>(items.size());

        for (size_t i = 0; i < items.size(); ++i)
        {
            result->items[i] = items[i].deep_copy();
        }

        return result;
    }


    size_t Array::hash() const noexcept
    {
        size_t hash_value = 0;

        for (const auto& value : items)
        {
            Value::hash_combine(hash_value, value.hash());
        }

        return hash_value;
    }


}

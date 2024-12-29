
#include "sorth-runtime.h"



namespace sorth::run_time::data_structures
{
    // When we print out a data structure we include the definition so that we can include field
    // names along with the name of the type itself.
    std::ostream& operator <<(std::ostream& stream, const StructurePtr& data)
    {
        if (data)
        {
            stream << "# " << data->definition->name << "\n";

            Value::value_format_indent += 4;

            for (size_t i = 0; i < data->fields.size(); ++i)
            {
                stream << std::string(Value::value_format_indent, ' ')
                       << data->definition->fieldNames[i] << " -> ";

                if (data->fields[i].is_string())
                {
                    stream << stringify(data->fields[i]);
                }
                else
                {
                    stream << data->fields[i];
                }

                if (i < data->fields.size() - 1)
                {
                    stream << " ,\n";
                }
            }

            Value::value_format_indent -= 4;

            stream << "\n" << std::string(Value::value_format_indent, ' ') << ";";
        }
        else
        {
            stream << "NULL";
        }

        return stream;
    }


    std::strong_ordering operator <=>(const Structure& lhs, const Structure& rhs)
    {
        if (lhs.definition->name != rhs.definition->name)
        {
            return lhs.definition->name <=> rhs.definition->name;
        }

        if (lhs.fields.size() != rhs.fields.size())
        {
            return lhs.fields.size() <=> rhs.fields.size();
        }

        for (size_t i = 0; i < lhs.fields.size(); ++i)
        {
            if (lhs.fields[i] != rhs.fields[i])
            {
                return lhs.fields[i] <=> rhs.fields[i];
            }
        }

        return std::strong_ordering::equal;
    }


    StructurePtr make_data_object(const StrucureDefinitionPtr& definition_ptr)
    {
        StructurePtr result = std::make_shared<Structure>();

        result->definition = definition_ptr;
        result->fields.reserve(definition_ptr->fieldNames.size());

        for (const auto& default_value : definition_ptr->defaults)
        {
            result->fields.push_back(default_value.deep_copy());
        }

        return result;
    }


    Value Structure::deep_copy() const noexcept
    {
        StructurePtr result = std::make_shared<Structure>();

        result->definition = definition;
        result->fields.reserve(fields.size());

        for (const auto& field : fields)
        {
            result->fields.push_back(field.deep_copy());
        }

        return result;
    }

    size_t Structure::hash() const noexcept
    {
        size_t hash_value = 0;

        for (const auto& field : fields)
        {
            Value::hash_combine(hash_value, field.hash());
        }

        return hash_value;
    }


}

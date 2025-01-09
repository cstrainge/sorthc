
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
                       << data->definition->field_names[i] << " -> ";

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


    uint8_t make_new_struct(const StrucureDefinitionPtr& definition_ptr, Value& output)
    {
        // Create an instance of the structure
        StructurePtr new_struct = std::make_shared<Structure>();

        new_struct->definition = definition_ptr;
        new_struct->fields.resize(definition_ptr->field_names.size());


        // Create an array of default values for the structure.  Then call the user's initialization
        // word to get the actual values.
        Value default_array = std::make_shared<Array>(definition_ptr->field_names.size());

        stack_push(&default_array);
        auto result = definition_ptr->init();

        // Make sure that the call to the word was successful.
        if (result)
        {
            set_last_error(("1 Structure " + definition_ptr->name +
                            " initialization failed.").c_str());
            return 1;
        }

        // Now pop the default values off the stack and assign them to the new structure.
        Value defaults;
        auto pop_result = stack_pop(&defaults);

        if (!defaults.is_array() || pop_result)
        {
            set_last_error(("2 Structure " + definition_ptr->name +
                            " initialization failed.").c_str());

            return 1;
        }

        ArrayPtr new_defaults = defaults.get_array();

        for (size_t i = 0; i < definition_ptr->field_names.size(); ++i)
        {
            new_struct->fields[i] = (*new_defaults)[i];
        }

        // The structure has now been initialized, so we can assign it to the output value and
        // return success.
        output = Value(new_struct);

        return 0;
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

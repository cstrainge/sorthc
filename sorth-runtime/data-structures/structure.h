
#pragma once



namespace sorth::run_time::data_structures
{


    using ValueList = std::vector<Value>;


    using FieldNameList = std::vector<std::string>;


    using InitFunction = std::function<uint8_t()>;


    // The base definition of a data object, useful for reflection and creation of the actual data
    // objects.
    struct StructureDefinition
    {
        std::string name;           // The name of the type.
        bool is_hidden;             // Is the structure and it's words hidden from the word list?
        FieldNameList field_names;  // Names of all the fields.
        InitFunction init;          // Function to initialize new instances of the structure.
    };


    using StrucureDefinitionPtr = std::shared_ptr<StructureDefinition>;


    class Structure
    {
        public:
            StrucureDefinitionPtr definition;   // Reference of the base definition.
            ValueList fields;                   // The actual values of the structure.

        public:
            Value deep_copy() const noexcept;
            size_t hash() const noexcept;
    };


    // Create a new data object for the given definition.
    uint8_t make_new_struct(const StrucureDefinitionPtr& definition_ptr, Value& output);


    // When we print out a data structure we include the definition so that we can include field
    // names along with the name of the type itself.
    std::ostream& operator <<(std::ostream& stream, const StructurePtr& data);


    std::strong_ordering operator <=>(const Structure& lhs, const Structure& rhs);


    inline bool operator ==(const Structure& lhs, const Structure& rhs)
    {
        return (lhs <=> rhs) == std::strong_ordering::equal;
    }

    inline bool operator !=(const Structure& lhs, const Structure& rhs)
    {
        return (lhs <=> rhs) != std::strong_ordering::equal;
    }


}

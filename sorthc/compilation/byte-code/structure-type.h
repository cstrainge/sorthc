
#pragma once



namespace sorthc::compilation::byte_code
{


    class Instruction;
    using ByteCode = std::vector<Instruction>;


    class StructureType
    {
        private:
            source::Location location;
            std::string name;
            WordVisibility visibility;
            std::vector<std::string> field_names;
            ByteCode initializer;

        public:
            StructureType(const source::Location& location,
                          const std::string& name,
                          WordVisibility visibility,
                          const std::vector<std::string>& field_names,
                          const ByteCode& initializer);
            StructureType(const StructureType& other) = default;
            StructureType(StructureType&& other) = default;
            ~StructureType() = default;

        public:
            StructureType& operator =(const StructureType& other) = default;
            StructureType& operator =(StructureType&& other) = default;

        public:
            const source::Location& get_location() const noexcept;
            const std::string& get_name() const noexcept;
            WordVisibility get_visibility() const noexcept;
            const std::vector<std::string>& get_field_names() const noexcept;
            const ByteCode& get_initializer() const noexcept;
    };


    using StructureTypeList = std::vector<StructureType>;


}

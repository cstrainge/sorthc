
#include "sorthc.h"



namespace sorth::compilation::byte_code
{


    StructureType::StructureType(const source::Location& location,
                          const std::string& name,
                          WordVisibility visibility,
                          const std::vector<std::string>& field_names,
                          const ByteCode& initializer)
    : location(location),
      name(name),
      visibility(visibility),
      field_names(field_names),
      initializer(initializer)
    {
    }


    StructureType::StructureType(const source::Location& location,
                                 const std::string& name,
                                 WordVisibility visibility,
                                 const std::vector<std::string>& field_names,
                                 const ByteCode& initializer,
                                 const FfiInfo& ffi_info)
    : location(location),
      name(name),
      visibility(visibility),
      field_names(field_names),
      initializer(initializer),
      ffi_info(ffi_info)
    {
    }


    const source::Location& StructureType::get_location() const noexcept
    {
        return location;
    }


    const std::string& StructureType::get_name() const noexcept
    {
        return name;
    }


    WordVisibility StructureType::get_visibility() const noexcept
    {
        return visibility;
    }


    const std::vector<std::string>& StructureType::get_field_names() const noexcept
    {
        return field_names;
    }


    const ByteCode& StructureType::get_initializer() const noexcept
    {
        return initializer;
    }


    const std::optional<FfiInfo>& StructureType::get_ffi_info() const noexcept
    {
        return ffi_info;
    }


}

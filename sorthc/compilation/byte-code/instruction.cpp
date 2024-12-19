
#include "sorthc.h"



namespace sorthc::compilation::byte_code
{


    Instruction::Instruction() noexcept
    : Instruction(Id::jump_target)
    {
    }


    Instruction::Instruction(const source::Location& location,
                             Id id,
                             const run_time::Value& value) noexcept
    : location(location),
      id(id),
      value(value)
    {
    }


    Instruction::Instruction(Id id, const run_time::Value& value) noexcept
    : location(),
      id(id),
      value(value)
    {
    }


    Instruction::Instruction(Id id) noexcept
    : location(),
      id(id),
      value()
    {
    }


    const std::optional<source::Location>& Instruction::get_location() const noexcept
    {
        return location;
    }


    Instruction::Id Instruction::get_id() const noexcept
    {
        return id;
    }


    const run_time::Value& Instruction::get_value() const noexcept
    {
        return value;
    }


}

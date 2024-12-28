
#include "sorthc.h"



namespace sorth::compilation::byte_code
{


    std::ostream& operator <<(std::ostream& stream, const Instruction::Id& id)
    {
        switch (id)
        {
            case Instruction::Id::def_variable:        stream << "def_variable       "; break;
            case Instruction::Id::def_constant:        stream << "def_constant       "; break;
            case Instruction::Id::read_variable:       stream << "read_variable      "; break;
            case Instruction::Id::write_variable:      stream << "write_variable     "; break;
            case Instruction::Id::execute:             stream << "execute            "; break;
            case Instruction::Id::word_index:          stream << "word_index         "; break;
            case Instruction::Id::word_exists:         stream << "word_exists        "; break;
            case Instruction::Id::push_constant_value: stream << "push_constant_value"; break;
            case Instruction::Id::mark_loop_exit:      stream << "mark_loop_exit     "; break;
            case Instruction::Id::unmark_loop_exit:    stream << "unmark_loop_exit   "; break;
            case Instruction::Id::mark_catch:          stream << "mark_catch         "; break;
            case Instruction::Id::unmark_catch:        stream << "unmark_catch       "; break;
            case Instruction::Id::mark_context:        stream << "mark_context       "; break;
            case Instruction::Id::release_context:     stream << "release_context    "; break;
            case Instruction::Id::jump:                stream << "jump               "; break;
            case Instruction::Id::jump_if_zero:        stream << "jump_if_zero       "; break;
            case Instruction::Id::jump_if_not_zero:    stream << "jump_if_not_zero   "; break;
            case Instruction::Id::jump_loop_start:     stream << "jump_loop_start    "; break;
            case Instruction::Id::jump_loop_exit:      stream << "jump_loop_exit     "; break;
            case Instruction::Id::jump_target:         stream << "jump_target        "; break;
        }

        return stream;
    }


    std::ostream& operator <<(std::ostream& stream, const Instruction& instruction)
    {
        auto doesnt_have_parameter = [](Instruction::Id id, const run_time::Value& value)
            {
                if (id == Instruction::Id::jump_target)
                {
                    return value.is_string() ? false : true;
                }

                return    (id == Instruction::Id::read_variable)
                       || (id == Instruction::Id::write_variable)
                       || (id == Instruction::Id::unmark_loop_exit)
                       || (id == Instruction::Id::unmark_catch)
                       || (id == Instruction::Id::mark_context)
                       || (id == Instruction::Id::release_context)
                       || (id == Instruction::Id::jump_loop_exit);
            };

        stream << instruction.get_id();

        if (!doesnt_have_parameter(instruction.get_id(), instruction.get_value()))
        {
            if (   (instruction.get_id() == Instruction::Id::push_constant_value)
                && (instruction.get_value().is_string()))
            {
                stream << "  " << stringify(instruction.get_value());
            }
            else
            {
                stream << "  " << instruction.get_value();
            }
        }

        return stream;
    }


    void pretty_print(std::ostream& stream,
                      run_time::CompilerRuntime& runtime,
                      const ByteCode& code)
    {
        for (size_t i = 0; i < code.size(); ++i)
        {
            stream << std::setw(6) << i << "  ";

            if (   (code[i].get_id() == Instruction::Id::execute)
                && (code[i].get_value().is_numeric()))
            {
                auto index = code[i].get_value().get_int(runtime);
                auto [ found, word_info ] = runtime.find(index);

                if (found)
                {
                    stream << code[i].get_id() << "  "
                           << word_info.name << ", (" << index << ")"
                           << std::endl;
                }
                else
                {
                    stream << code[i].get_id() << "  " << index << std::endl;
                }
            }
            else if (   (code[i].get_id() == Instruction::Id::push_constant_value)
                        && (code[i].get_value().is_string()))
            {
                auto string = code[i].get_value().get_string(runtime);

                stream << code[i].get_id() << "  " << run_time::stringify(string) << std::endl;
            }
            else if (  (   (code[i].get_id() == Instruction::Id::mark_loop_exit)
                        || (code[i].get_id() == Instruction::Id::mark_catch)
                        || (code[i].get_id() == Instruction::Id::jump)
                        || (code[i].get_id() == Instruction::Id::jump_if_zero)
                        || (code[i].get_id() == Instruction::Id::jump_if_not_zero))
                        && (code[i].get_value().is_numeric()))
            {
                auto offset = code[i].get_value().get_int(runtime);

                stream << code[i].get_id() << "  " << i + offset << std::endl;
            }
            else
            {
                stream << code[i] << std::endl;
            }
        }
    }


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


    run_time::Value& Instruction::get_value() noexcept
    {
        return value;
    }


}

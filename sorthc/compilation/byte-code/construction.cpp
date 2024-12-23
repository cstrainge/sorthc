
#include "sorthc.h"



namespace sorthc::compilation::byte_code
{


    Construction::Construction() noexcept
    : execution_context(WordExecutionContext::run_time),
      visibility(WordVisibility::visible),
      context_management(WordContextManagement::managed)
    {
    }


    Construction::Construction(const source::Location& location, const std::string& name) noexcept
    : Construction()
    {
        this->location = location;
        this->name = name;
    }


    Construction::Construction(const source::Location& location,
                               const std::string& name,
                               const ByteCode&& code) noexcept
    : Construction()
    {
        this->location = location;
        this->name = name;
        this->code = std::move(code);
    }


    Construction::Construction(const ByteCode&& code) noexcept
    : Construction()
    {
        this->code = code;
    }


    WordExecutionContext Construction::get_execution_context() const noexcept
    {
        return execution_context;
    }


    void Construction::set_execution_context(WordExecutionContext value) noexcept
    {
        execution_context = value;
    }


    WordVisibility Construction::get_visibility() const noexcept
    {
        return visibility;
    }


    void Construction::set_visibility(WordVisibility value) noexcept
    {
        visibility = value;
    }


    WordContextManagement Construction::get_context_management() const noexcept
    {
        return context_management;
    }


    void Construction::set_context_management(WordContextManagement value) noexcept
    {
        context_management = value;
    }


    const source::Location& Construction::get_location() const noexcept
    {
        return location;
    }


    void Construction::set_location(const source::Location& value) noexcept
    {
        location = value;
    }


    const std::string& Construction::get_name() const noexcept
    {
        return name;
    }


    void Construction::set_name(const std::string& value) noexcept
    {
        name = value;
    }


    const std::string& Construction::get_description() const noexcept
    {
        return description;
    }


    void Construction::set_description(const std::string& value) noexcept
    {
        description = value;
    }


    const std::string& Construction::get_signature() const noexcept
    {
        return signature;
    }


    void Construction::set_signature(const std::string& value) noexcept
    {
        signature = value;
    }


    ByteCode& Construction::get_code() noexcept
    {
        return code;
    }


    const ByteCode& Construction::get_code() const noexcept
    {
        return code;
    }


    ByteCode&& Construction::take_code() noexcept
    {
        return std::move(code);
    }


    void Construction::set_code(const ByteCode& value) noexcept
    {
        code = value;
    }


    void Construction::set_code(ByteCode&& value) noexcept
    {
        code = std::move(value);
    }


}

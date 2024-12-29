
#include "sorthc.h"



namespace sorth::compilation
{


    Word::Word(WordExecutionContext context,
               WordType type,
               WordVisibility visibility,
               WordContextManagement management,
               const std::string& name,
               const source::Location& location,
               size_t handler_index) noexcept
    : context(context),
      type(type),
      visibility(visibility),
      management(management),
      name(name),
      location(location),
      handler_index(handler_index)
    {
    }


    WordExecutionContext Word::get_context() const noexcept
    {
        return context;
    }


    WordType Word::get_type() const noexcept
    {
        return type;
    }


    WordVisibility Word::get_visibility() const noexcept
    {
        return visibility;
    }


    WordContextManagement Word::get_management() const noexcept
    {
        return management;
    }


    const std::string& Word::get_name() const noexcept
    {
        return name;
    }


    const source::Location& Word::get_location() const noexcept
    {
        return location;
    }


    size_t Word::get_handler_index() const noexcept
    {
        return handler_index;
    }


}

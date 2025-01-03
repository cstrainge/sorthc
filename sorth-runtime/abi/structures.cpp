
#include "sorth-runtime.h"
#include "structures.h"



using namespace sorth::run_time::data_structures;


namespace
{


    std::unordered_map<std::string, StrucureDefinitionPtr> structure_definitions;


}


extern "C"
{


    void register_structure_type(const char* name,
                                 const char* fields[],
                                 size_t field_count,
                                 uint8_t (*init_function)(void))
    {
        auto new_type = std::make_shared<StructureDefinition>();

        new_type->name = name;
        new_type->is_hidden = false;
        new_type->init = init_function;

        new_type->field_names.reserve(field_count);

        for (size_t i = 0; i < field_count; ++i)
        {
            new_type->field_names.push_back(fields[i]);
        }

        structure_definitions[name] = new_type;
    }


}



namespace sorth::run_time::abi
{


    uint8_t create_structure(const std::string& name, Value* output)
    {
        auto iterator = structure_definitions.find(name);

        if (iterator == structure_definitions.end())
        {
            set_last_error("Unknown structure type.");

            return 1;
        }

        return make_new_struct(iterator->second, *output);
    }


}

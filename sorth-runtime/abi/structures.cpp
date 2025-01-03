
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
std::cout << "register_structure_type" << std::endl;
        auto new_type = std::make_shared<StructureDefinition>();

std::cout << "----<00>--------"<< std::endl;
        new_type->name = name;
std::cout << "----<01>--------"<< std::endl;
        new_type->is_hidden = false;
std::cout << "----<02>--------"<< std::endl;
        new_type->init = init_function;

std::cout << "----<03>--------"<< std::endl;
        new_type->field_names.reserve(field_count);

std::cout << "----<04>-------- " << (void*)fields << std::endl;
        for (size_t i = 0; i < field_count; ++i)
        {
std::cout << "----<05>-------- " << (void*)fields[i] /*<< fields[i]*/ << std::endl;
            new_type->field_names.push_back(fields[i]);
        }

std::cout << "----<06>--------"<< std::endl;
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

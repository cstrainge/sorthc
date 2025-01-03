
#pragma once



extern "C"
{


    // Called by the generated code to register new structure types with the run-time at startup.
    void register_structure_type(const char* name,
                                 const char* fields[],
                                 size_t field_count,
                                 uint8_t (*init_function)(void));


}


namespace sorth::run_time::abi
{


    // Called internally by the C++ run-time to create and initialize a new structure object for use
    // by the generated code.
    uint8_t create_structure(const std::string& name,
                             sorth::run_time::data_structures::Value* output);


}

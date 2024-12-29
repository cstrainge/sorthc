
#pragma once


extern "C"
{


    // Initialize a variable with the default value that had been allocated on the stack by the
    // generated code.
    void initialize_variable(sorth::run_time::data_structures::Value* value) noexcept;


    // Free any extra memory used by the variable that had been allocated on the stack by the
    // generated code.
    void free_variable(sorth::run_time::data_structures::Value* value) noexcept;


    // Allocate a new reference slab of variables that have been allocated by the generated code
    // on the stack.
    void allocate_variable_slab(sorth::run_time::data_structures::Value* slab[],
                                size_t size) noexcept;


    // As the generated code exits a block, it will release the slab of variables that wre part
    // of that block.
    void release_variable_slab() noexcept;


    // Search the list of slabs for the variable by index and return the value.
    void read_variable(size_t index, sorth::run_time::data_structures::Value* output) noexcept;


    // Search the list of slabs for the variable by index and write the value.
    void write_variable(size_t index, sorth::run_time::data_structures::Value* value) noexcept;


    // Called by generated code to copy the value of one variable to another.
    void copy_variable(sorth::run_time::data_structures::Value* input,
                       sorth::run_time::data_structures::Value* output) noexcept;


}

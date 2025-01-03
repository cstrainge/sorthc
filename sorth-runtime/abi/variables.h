
#pragma once



extern "C"
{


    // Initialize a variable with the default value that had been allocated on the stack by the
    // generated code.
    void initialize_variable(sorth::run_time::data_structures::Value* value) noexcept;


    // Free any extra memory used by the variable that had been allocated on the stack by the
    // generated code.
    void free_variable(sorth::run_time::data_structures::Value* value) noexcept;


    // Allocate a new reference block of variables that have been allocated by the generated code
    // on the stack.  Returns the base index of the block of variables.
    int64_t allocate_variable_block(sorth::run_time::data_structures::Value* block[],
                                    size_t size) noexcept;


    // As the generated code exits a block, it will release the slab of variables that wre part
    // of that block.
    void release_variable_block() noexcept;


    // Search the list of slabs for the variable by index and return the value.
    bool read_variable(size_t index, sorth::run_time::data_structures::Value* output) noexcept;


    // Search the list of slabs for the variable by index and write the value.
    bool write_variable(size_t index, sorth::run_time::data_structures::Value* value) noexcept;


    // Called by generated code to copy the value of one variable to another.
    void deep_copy_variable(sorth::run_time::data_structures::Value* input,
                            sorth::run_time::data_structures::Value* output) noexcept;


}

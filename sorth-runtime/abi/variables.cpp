
#include "sorth-runtime.h"



using namespace sorth::run_time::data_structures;


namespace sorth::run_time::abi
{


    namespace
    {


        // Keep track of the variables that are allocated on the stack in the Forth program.
        //
        // The user code will allocate a block of variables, and if it needs to access the variable
        // list by index, it will use these slabs to access the variables.
        class VariableBlock
        {
            private:
                // A block of variables that have been allocated on the stack.
                struct Block
                {
                    size_t start;    // The index of the first variable in the block.
                    size_t size;     // The number of variables in the block.

                    Value** values;  // Pointers to the variables in the block.
                };

            private:
                // The stack of slabs that have been allocated.
                std::list<Block> slabs;

            public:
                // Allocate a new block of variables on the stack.
                void allocate(Value* block[], size_t size) noexcept
                {
                    size_t start = 0;

                    if (!slabs.empty())
                    {
                        start = slabs.front().start + slabs.front().size;
                    }

                    slabs.push_front({.start = start, .size = size, .values = block});
                }

                // Release the most recently allocated block of variables.
                void release() noexcept
                {
                    if (!slabs.empty())
                    {
                        slabs.pop_front();
                    }
                }

                // Get a variable from one of the blocks by index.
                Value* get(size_t index) noexcept
                {
                    for (const auto& block : slabs)
                    {
                        if (   (index >= block.start)
                            && (index < (block.start + block.size)))
                        {
                            return block.values[index - block.start];
                        }
                    }

                    return nullptr;
                }
        };


        // Each thread get's it's own set of variables to go with it's own stack.
        thread_local VariableBlock variables;


    }


}


using namespace sorth::run_time::abi;


extern "C"
{


    // Initialize a variable with the default value that had been allocated on the stack by the
    // generated code.
    void initialize_variable(Value* value) noexcept
    {
        new (value) Value();
    }


    // Free any extra memory used by the variable that had been allocated on the stack by the
    // generated code.
    void free_variable(Value* value) noexcept
    {
        value->~Value();
    }


    // Allocate a new reference block of variables that have been allocated by the generated code
    // on the stack.
    void allocate_variable_slab(Value* block[], size_t size) noexcept
    {
        variables.allocate(block, size);
    }


    // As the generated code exits a block, it will release the block of variables that wre part
    // of that block.
    void release_variable_slab() noexcept
    {
        variables.release();
    }


    // Search the list of slabs for the variable by index and return the value.
    void read_variable(size_t index, Value* output) noexcept
    {
        (*output) = (*variables.get(index));
    }


    // Search the list of slabs for the variable by index and write the value.
    void write_variable(size_t index, Value* value) noexcept
    {
        (*variables.get(index)) = (*value);
    }


    // Called by generated code to copy the value of one variable to another.
    void copy_variable(Value* input, Value* output) noexcept
    {
        (*output) = (*input);
    }


}

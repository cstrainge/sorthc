
#include "sorth-runtime.h"



using namespace sorth::run_time::data_structures;


namespace sorth::run_time::abi
{


    namespace
    {


        // Keep track of the variables that are allocated on the stack in the Forth program.
        //
        // The user code will allocate a slab of variables, and if it needs to access the variable
        // list by index, it will use these slabs to access the variables.
        class VariableSlab
        {
            private:
                // A slab of variables that have been allocated on the stack.
                struct Slab
                {
                    size_t start;    // The index of the first variable in the slab.
                    size_t size;     // The number of variables in the slab.

                    Value** values;  // Pointers to the variables in the slab.
                };

            private:
                // The stack of slabs that have been allocated.
                std::list<Slab> slabs;

            public:
                // Allocate a new slab of variables on the stack.
                void allocate(Value* slab[], size_t size) noexcept
                {
                    size_t start = 0;

                    if (!slabs.empty())
                    {
                        start = slabs.front().start + slabs.front().size;
                    }

                    slabs.push_front({.start = start, .size = size, .values = slab});
                }

                // Release the most recently allocated slab of variables.
                void release() noexcept
                {
                    if (!slabs.empty())
                    {
                        slabs.pop_front();
                    }
                }

                // Read a variable from the slab by index.
                Value* read(size_t index) noexcept
                {
                    for (const auto& slab : slabs)
                    {
                        if (   (index >= slab.start)
                            && (index < (slab.start + slab.size)))
                        {
                            return slab.values[index - slab.start];
                        }
                    }

                    return nullptr;
                }

                // Write a variable to the slab by index.
                void write(size_t index, Value* value) noexcept
                {
                    for (const auto& slab : slabs)
                    {
                        if (   (index >= slab.start)
                            && (index < (slab.start + slab.size)))
                        {
                            (*slab.values[index - slab.start]) = (*value);
                        }
                    }
                }
        };


        // Each thread get's it's own set of variables to go with it's own stack.
        thread_local VariableSlab variables;


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


    // Allocate a new reference slab of variables that have been allocated by the generated code
    // on the stack.
    void allocate_variable_slab(Value* slab[], size_t size) noexcept
    {
        variables.allocate(slab, size);
    }


    // As the generated code exits a block, it will release the slab of variables that wre part
    // of that block.
    void release_variable_slab() noexcept
    {
        variables.release();
    }


    // Search the list of slabs for the variable by index and return the value.
    void read_variable(size_t index, Value* output) noexcept
    {
        (*output) = (*variables.read(index));
    }


    // Search the list of slabs for the variable by index and write the value.
    void write_variable(size_t index, Value* value) noexcept
    {
        variables.write(index, value);
    }


    // Called by generated code to copy the value of one variable to another.
    void copy_variable(Value* input, Value* output) noexcept
    {
        (*output) = (*input);
    }


}

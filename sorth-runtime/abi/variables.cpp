
#include "sorth-runtime.h"



using namespace sorth::run_time::data_structures;


namespace sorth::run_time::abi
{


    namespace
    {


        class VariableSlab
        {
            public:
                void allocate(Value* slab[], size_t size) noexcept
                {
                }

                void release() noexcept
                {
                }

                Value* read(size_t index) noexcept
                {
                    return nullptr;
                }

                void write(size_t index, Value* value) noexcept
                {
                }
        };


        thread_local VariableSlab variables;


    }


}


using namespace sorth::run_time::abi;


extern "C"
{


    void initialize_variable(Value* value) noexcept
    {
        new (value) Value();
    }


    void free_variable(Value* value) noexcept
    {
        value->~Value();
    }


    void allocate_variable_slab(Value* slab[], size_t size) noexcept
    {
        variables.allocate(slab, size);
    }


    void release_variable_slab() noexcept
    {
        variables.release();
    }


    Value* read_variable(size_t index) noexcept
    {
        return variables.read(index);
    }


    void write_variable(size_t index, Value* value) noexcept
    {
        variables.write(index, value);
    }


}

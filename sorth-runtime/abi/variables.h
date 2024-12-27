
#pragma once


extern "C"
{


    void initialize_variable(sorth::run_time::data_structures::Value* value) noexcept;


    void free_variable(sorth::run_time::data_structures::Value* value) noexcept;


    void allocate_variable_slab(sorth::run_time::data_structures::Value* slab[],
                                size_t size) noexcept;


    void release_variable_slab() noexcept;


    sorth::run_time::data_structures::Value* read_variable(size_t index) noexcept;


    void write_variable(size_t index, sorth::run_time::data_structures::Value* value) noexcept;


}

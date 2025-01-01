
#include "sorth-runtime.h"
#include "hash-table-words.h"



using namespace sorth::run_time::data_structures;



namespace
{


    HashTablePtr stack_pop_as_hash_table()
    {
        Value value;

        auto pop_result = stack_pop(&value);

        if (pop_result)
        {
            return nullptr;
        }

        if (!value.is_hash_table())
        {
            set_last_error("Expected a hash table value.");
            return nullptr;
        }

        return value.get_hash_table();
    }


}


extern "C"
{


        using WordType = int8_t (*)();
        extern WordType word_table[];


        uint8_t word_hash_table_new()
        {
            auto table = std::make_shared<HashTable>();
            auto value = Value(table);

            stack_push(&value);

            return 0;
        }


        uint8_t word_hash_table_insert()
        {
            auto table = stack_pop_as_hash_table();
            Value key;
            Value value;

            auto pop_result_1 = stack_pop(&key);
            auto pop_result_2 = stack_pop(&value);

            if ((!table) || pop_result_1 || pop_result_2)
            {
                return 1;
            }

            table->insert(key, value);

            return 0;
        }


        uint8_t word_hash_table_find()
        {
            auto table = stack_pop_as_hash_table();
            Value key;

            auto pop_result = stack_pop(&key);

            if ((!table) || pop_result)
            {
                return 1;
            }

            auto [ found, value ] = table->get(key);

            if (!found)
            {
                std::stringstream stream;

                stream << "Value, " << key << ", does not exist in the table.";
                set_last_error(stream.str().c_str());

                return 1;
            }

            stack_push(&value);

            return 0;
        }


        uint8_t word_hash_table_exists()
        {
            auto table = stack_pop_as_hash_table();
            Value key;

            auto pop_result = stack_pop(&key);

            if ((!table) || pop_result)
            {
                return 1;
            }

            auto [ found, value ] = table->get(key);

            stack_push_bool(found);

            return 0;
        }


        uint8_t word_hash_plus()
        {
            auto hash_src = stack_pop_as_hash_table();
            auto hash_dest = stack_pop_as_hash_table();

            if (!hash_src || !hash_dest)
            {
                return 1;
            }

            for (auto entry : hash_src->get_items())
            {
                auto key = entry.first;
                auto value = entry.second;

                hash_dest->insert(key.deep_copy(), value.deep_copy());
            }

            Value value = hash_dest;

            stack_push(&value);

            return 0;
        }


        uint8_t word_hash_compare()
        {
            auto hash_a = stack_pop_as_hash_table();
            auto hash_b = stack_pop_as_hash_table();

            if (!hash_a || !hash_b)
            {
                return 1;
            }

            stack_push_bool(hash_a == hash_b);

            return 0;
        }


        uint8_t word_hash_table_size()
        {
            auto hash = stack_pop_as_hash_table();

            if (!hash)
            {
                return 1;
            }

            stack_push_int(hash->size());

            return 0;
        }


        uint8_t word_hash_table_iterate()
        {
            auto table = stack_pop_as_hash_table();
            int64_t word_index;

            auto pop_result = stack_pop_int(&word_index);

            if ((!table) || pop_result)
            {
                return 1;
            }

            auto& handler = word_table[word_index];

            for (const auto& item : table->get_items())
            {
                stack_push(&item.first);
                stack_push(&item.second);

                auto result = handler();

                if (result)
                {
                    return result;
                }
            }

            return 0;
        }


}


namespace sorth::run_time::abi::words
{


    void register_hash_table_words(const RuntimeWordRegistrar& registrar)
    {
        registrar("{}.new", "word_hash_table_new");
        registrar("{}!", "word_hash_table_insert");
        registrar("{}@", "word_hash_table_find");
        registrar("{}?", "word_hash_table_exists");
        registrar("{}.+", "word_hash_plus");
        registrar("{}.=", "word_hash_compare");
        registrar("{}.size@", "word_hash_table_size");
        registrar("{}.iterate", "word_hash_table_iterate");
    }


}

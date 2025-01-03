
#include "sorthc.h"



extern "C"
{


    // Dummy version of the word table, which will be properly defined in the generated code.
    // However the runtime library has an external reference to it, so we need to define it here as
    // well.
    using WordType = int8_t (*)();
    WordType word_table[1] = { nullptr };


}


namespace sorth::compilation
{


    namespace
    {


        // LLVM IR has rules for what characters are allowed in a symbol name.  This function will
        // filter out any characters that are not allowed.
        //
        // Additionally, we'll add unique naming and an index to ensure that duplicate names don't
        // interfere with each other.  It's valid to have multiple words with the same name in
        // Forth.
        std::string filter_word_name(const std::string& name)
        {
            static std::atomic<int64_t> index = 0;

            std::stringstream stream;
            auto current = index.fetch_add(1, std::memory_order_relaxed);

            stream << "_word_fn_";

            for (auto c : name)
            {
                switch (c)
                {
                    case '@':   stream << "_at";            break;
                    case '\'':  stream << "_prime";         break;
                    case '"':   stream << "_quote";         break;
                    case '%':   stream << "_percent";       break;
                    case '!':   stream << "_bang";          break;
                    case '?':   stream << "_question";      break;
                    case '=':   stream << "_equal";         break;
                    case '<':   stream << "_less";          break;
                    case '>':   stream << "_greater";       break;
                    case '+':   stream << "_plus";          break;
                    case '[':   stream << "_left_square";   break;
                    case ']':   stream << "_right_square";  break;

                    default:    stream << c;            break;
                }
            }

            stream << "_"
                   << std::setw(6) << std::setfill('0') << current
                   << "_";

            return stream.str();
        }


        // Map of word names to their indicies in the word list.
        using WordMap = std::unordered_map<std::string, size_t>;


        // Map of names to LLVM global variables.
        using GlobalMap = std::unordered_map<std::string, llvm::GlobalVariable*>;


        struct ValueInfo
        {
            llvm::AllocaInst* variable;
            llvm::AllocaInst* variable_index;
            size_t block_index;
        };


        // Map of names ot LLVM local variables.
        using ValueMap = std::unordered_map<std::string, ValueInfo>;


        // Information about a word that the compiler knows about.
        struct WordInfo
        {
            std::string name;           // The Forth name of the word.
            std::string handler_name;   // The name of the function that implements the word.
            bool was_referenced;        // True if the word was referenced by the script.

            std::optional<byte_code::ByteCode> code;  // If unassigned, then it's a native word.

            llvm::Function* function;   // The compiled function or declaration for the word.
        };


        // List of words as we find them from various sources.
        using WordInfoList = std::vector<WordInfo>;


        struct WordCollection;
        void try_resolve_calls(WordCollection& collection, byte_code::ByteCode& code);

        void mark_used_words(WordCollection& collection, const byte_code::ByteCode& code);


        // Collection of words that the compiler knows about.  They can come from the runtime, the
        // standard library, or the program being compiled.
        struct WordCollection
        {
            byte_code::StructureTypeList structures;

            WordInfoList words;
            WordMap word_map;

            // Add a structure to our collection.
            void add_structure(const byte_code::StructureType& structure)
            {
                structures.push_back(structure);
            }

            // Add a native word to our collection.
            void add_word(const std::string& name, const std::string& handler_name)
            {
                WordInfo info;

                info.name = name;
                info.handler_name = handler_name;
                info.was_referenced = false;
                // Code is not assigned because it's a native word.
                info.function = nullptr;

                add_word(std::move(info));
            }

            // Add a Forth word to our collection.
            void add_word(const byte_code::Construction& word)
            {
                // Cache the word's name.
                const auto& name = word.get_name();

                // Copy over the word's information that we care about at this level.
                WordInfo info;

                info.name = name;
                info.handler_name = filter_word_name(name);
                info.code = word.get_code();
                info.was_referenced = false;
                info.function = nullptr;

                // Try to resolve any calls in the word's code.  We know for sure that some calls
                // will remain unresolved, for example calls to constant and variable index words.
                try_resolve_calls(*this, info.code.value());

                // Add to the collections.
                add_word(std::move(info));
            }

            // Add a completed word info to the collection.
            void add_word(WordInfo&& word_info)
            {
                auto name = word_info.name;

                words.emplace_back(word_info);
                word_map[name] = words.size() - 1;
            }
        };


        // Try to resolve calls to words in the word's code.  This way if a word is overwritten at a
        // later point, this word will still call the correct word.
        void try_resolve_calls(WordCollection& collection, byte_code::ByteCode& code)
        {
            for (auto& instruction : code)
            {
                // First make sure we're dealing with an execute instruction.
                if (instruction.get_id() == byte_code::Instruction::Id::execute)
                {
                    // If the value is still unresolved...
                    if (instruction.get_value().is_string())
                    {
                        // Try to resolve the word name to it's index.
                        auto& name = instruction.get_value().get_string();
                        auto iterator = collection.word_map.find(name);

                        if (iterator != collection.word_map.end())
                        {
                            // We found the word, so update the instruction to call the word by it's
                            // index.  We don't mark this word as referenced yet, because we don't
                            // know if the word we're resolving is actually referenced.
                            instruction.get_value() = iterator->second;
                        }
                    }
                }
            }
        }


        // Try to resolve all the outstanding calls in the collection of words.
        void try_resolve_words(WordCollection& collection)
        {
            // Go through the collection of words...
            for (auto& word : collection.words)
            {
                // If the word isn't a native one...
                if (word.code.has_value())
                {
                    // Try to resolve all the outstanding calls in the word's code.
                    try_resolve_calls(collection, word.code.value());
                }
            }
        }


        // The API as exposed by the runtime-library that's intended to be called directly by the
        // generated code.
        struct RuntimeApi
        {
            // The value structure type.
            llvm::StructType* value_struct_type;
            llvm::PointerType* value_struct_ptr_type;
            llvm::ArrayType* value_struct_ptr_array_type;

            // External variable functions.
            llvm::Function* initialize_variable;
            llvm::Function* free_variable;
            llvm::Function* allocate_variable_block;
            llvm::Function* release_variable_block;
            llvm::Function* read_variable;
            llvm::Function* write_variable;
            llvm::Function* deep_copy_variable;

            // External stack functions.
            llvm::Function* stack_push;
            llvm::Function* stack_push_int;
            llvm::Function* stack_push_double;
            llvm::Function* stack_push_bool;
            llvm::Function* stack_push_string;
            llvm::Function* stack_pop;
            llvm::Function* stack_pop_int;
            llvm::Function* stack_pop_bool;

            // User structure functions.
            llvm::Function* register_structure_type;

            // External error functions.
            llvm::Function* set_last_error;
            llvm::Function* get_last_error;
            llvm::Function* push_last_error;
            llvm::Function* clear_last_error;
        };


        // Register the runtime API with the module.
        RuntimeApi register_runtime_api(std::shared_ptr<llvm::Module>& module)
        {
            const size_t value_size = sizeof(sorth::run_time::data_structures::Value);

            auto void_type = llvm::Type::getVoidTy(module->getContext());
            auto uint64_type = llvm::Type::getInt64Ty(module->getContext());
            auto double_type = llvm::Type::getDoubleTy(module->getContext());
            auto bool_type = llvm::Type::getInt1Ty(module->getContext());
            auto char_ptr_type = llvm::PointerType::getUnqual(bool_type);

            auto uint64_ptr_type = llvm::PointerType::getUnqual(uint64_type);

            auto byte_type = llvm::Type::getInt8Ty(module->getContext());
            auto byte_array_type = llvm::ArrayType::get(byte_type, value_size);

            // Register the value struck type as an opaque data type.
            std::vector<llvm::Type*> value_struct_members = { byte_array_type };

            auto value_struct_type = llvm::StructType::create(module->getContext(),
                                                              value_struct_members,
                                                              "Value");

            auto value_struct_ptr_type = llvm::PointerType::getUnqual(value_struct_type);

            auto value_struct_ptr_array_type = llvm::ArrayType::get(value_struct_ptr_type, 0);

            auto init_function_type = llvm::FunctionType::get(void_type, false);
            auto init_function_ptr_type = llvm::PointerType::getUnqual(init_function_type);

            // Register the external variable functions.
            auto initialize_variable_signature
                             = llvm::FunctionType::get(void_type, { value_struct_ptr_type }, false);
            auto initialize_variable = llvm::Function::Create(initialize_variable_signature,
                                                              llvm::Function::ExternalLinkage,
                                                              "initialize_variable",
                                                              module.get());

            auto free_variable = llvm::Function::Create(initialize_variable_signature,
                                                        llvm::Function::ExternalLinkage,
                                                        "free_variable",
                                                        module.get());

            auto allocate_variable_block_signature
                             = llvm::FunctionType::get(uint64_type,
                                                       { value_struct_ptr_type, uint64_type },
                                                       false);
            auto allocate_variable_block = llvm::Function::Create(allocate_variable_block_signature,
                                                                 llvm::Function::ExternalLinkage,
                                                                 "allocate_variable_block",
                                                                 module.get());

            auto release_variable_block_signature = llvm::FunctionType::get(void_type, false);
            auto release_variable_block = llvm::Function::Create(release_variable_block_signature,
                                                                llvm::Function::ExternalLinkage,
                                                                "release_variable_block",
                                                                module.get());

            auto rw_variable_signature =
                                     llvm::FunctionType::get(bool_type,
                                                             { uint64_type, value_struct_ptr_type },
                                                             false);

            auto read_variable = llvm::Function::Create(rw_variable_signature,
                                                        llvm::Function::ExternalLinkage,
                                                        "read_variable",
                                                        module.get());
            auto write_variable = llvm::Function::Create(rw_variable_signature,
                                                         llvm::Function::ExternalLinkage,
                                                         "write_variable",
                                                         module.get());

            auto deep_copy_variable_signature = llvm::FunctionType::get(void_type,
                                                                  { value_struct_ptr_type,
                                                                    value_struct_ptr_type },
                                                                  false);

            auto deep_copy_variable = llvm::Function::Create(deep_copy_variable_signature,
                                                        llvm::Function::ExternalLinkage,
                                                        "deep_copy_variable",
                                                        module.get());

            // Register the external stack functions.
            auto stack_push_signature = llvm::FunctionType::get(void_type,
                                                                { value_struct_ptr_type },
                                                                false);
            auto stack_push = llvm::Function::Create(stack_push_signature,
                                                     llvm::Function::ExternalLinkage,
                                                     "stack_push",
                                                     module.get());

            auto stack_push_int_signature = llvm::FunctionType::get(void_type,
                                                                    { uint64_type },
                                                                    false);
            auto stack_push_int = llvm::Function::Create(stack_push_int_signature,
                                                         llvm::Function::ExternalLinkage,
                                                         "stack_push_int",
                                                         module.get());

            auto stack_push_double_signature = llvm::FunctionType::get(void_type,
                                                                       { double_type },
                                                                       false);
            auto stack_push_double = llvm::Function::Create(stack_push_double_signature,
                                                            llvm::Function::ExternalLinkage,
                                                            "stack_push_double",
                                                            module.get());

            auto stack_push_bool_signature = llvm::FunctionType::get(void_type,
                                                                     { bool_type },
                                                                     false);
            auto stack_push_bool = llvm::Function::Create(stack_push_bool_signature,
                                                          llvm::Function::ExternalLinkage,
                                                          "stack_push_bool",
                                                          module.get());

            auto stack_push_string_signature = llvm::FunctionType::get(void_type,
                                                                       { char_ptr_type },
                                                                       false);
            auto stack_push_string = llvm::Function::Create(stack_push_string_signature,
                                                            llvm::Function::ExternalLinkage,
                                                            "stack_push_string",
                                                            module.get());

            auto stack_pop_signature = llvm::FunctionType::get(bool_type,
                                                               { value_struct_ptr_type },
                                                               false);
            auto stack_pop = llvm::Function::Create(stack_pop_signature,
                                                    llvm::Function::ExternalLinkage,
                                                    "stack_pop",
                                                    module.get());

            auto stack_pop_int_signature = llvm::FunctionType::get(bool_type,
                                                                   { uint64_ptr_type },
                                                                   false);
            auto stack_pop_int = llvm::Function::Create(stack_pop_int_signature,
                                                        llvm::Function::ExternalLinkage,
                                                        "stack_pop_int",
                                                        module.get());

            auto stack_pop_bool_signature = llvm::FunctionType::get(bool_type,
                                                                    { char_ptr_type },
                                                                    false);
            auto stack_pop_bool = llvm::Function::Create(stack_pop_bool_signature,
                                                         llvm::Function::ExternalLinkage,
                                                         "stack_pop_bool",
                                                         module.get());

            // Register the user structure functions.
            auto register_structure_type_signature = llvm::FunctionType::get(void_type,
                                                                {
                                                                    char_ptr_type,
                                                                    char_ptr_type,
                                                                    uint64_type,
                                                                    init_function_ptr_type
                                                                },
                                                                false);
            auto register_structure_type = llvm::Function::Create(register_structure_type_signature,
                                                                  llvm::Function::ExternalLinkage,
                                                                  "register_structure_type",
                                                                  module.get());

            // Register the external error functions.
            auto set_last_error_signature = llvm::FunctionType::get(void_type,
                                                                    { char_ptr_type },
                                                                    false);
            auto set_last_error = llvm::Function::Create(set_last_error_signature,
                                                        llvm::Function::ExternalLinkage,
                                                        "set_last_error",
                                                        module.get());

            auto get_last_error_signature = llvm::FunctionType::get(char_ptr_type, false);
            auto get_last_error = llvm::Function::Create(get_last_error_signature,
                                                         llvm::Function::ExternalLinkage,
                                                         "get_last_error",
                                                         module.get());

            auto push_last_error_signature = llvm::FunctionType::get(void_type, false);
            auto push_last_error = llvm::Function::Create(push_last_error_signature,
                                                          llvm::Function::ExternalLinkage,
                                                          "push_last_error",
                                                          module.get());

            auto clear_last_error_signature = llvm::FunctionType::get(void_type, false);
            auto clear_last_error = llvm::Function::Create(clear_last_error_signature,
                                                           llvm::Function::ExternalLinkage,
                                                           "clear_last_error",
                                                           module.get());

            return
                {
                    .value_struct_type = value_struct_type,
                    .value_struct_ptr_type = value_struct_ptr_type,
                    .value_struct_ptr_array_type = value_struct_ptr_array_type,

                    .initialize_variable = initialize_variable,
                    .free_variable = free_variable,
                    .allocate_variable_block = allocate_variable_block,
                    .release_variable_block = release_variable_block,
                    .read_variable = read_variable,
                    .write_variable = write_variable,
                    .deep_copy_variable = deep_copy_variable,

                    .stack_push = stack_push,
                    .stack_push_int = stack_push_int,
                    .stack_push_double = stack_push_double,
                    .stack_push_bool = stack_push_bool,
                    .stack_push_string = stack_push_string,
                    .stack_pop = stack_pop,
                    .stack_pop_int = stack_pop_int,
                    .stack_pop_bool = stack_pop_bool,

                    .register_structure_type = register_structure_type,

                    .set_last_error = set_last_error,
                    .get_last_error = get_last_error,
                    .push_last_error = push_last_error,
                    .clear_last_error = clear_last_error
                };
        }


        // Gather all the runtime words into the collection.
        void gather_runtime_words(WordCollection& collection)
        {
            auto collector = [&collection](const std::string& name, const std::string& handler_name)
                {
                    collection.add_word(name, handler_name);
                };

            sorth::run_time::abi::words::register_runtime_words(collector);
        }


        // Gather all the words in the script and it's subscripts.
        void gather_script_words(const byte_code::ScriptPtr& script, WordCollection& collection)
        {
            // Gather up the words in the sub-scripts first...
            for (const auto& sub_script : script->get_sub_scripts())
            {
                gather_script_words(sub_script, collection);
            }

            // Now we gather up the words in this script.
            for (const auto& word : script->get_words())
            {
                collection.add_word(word);
            }
        }


        // Create the structure init and access words for the script and it's subscripts.
        void create_structure_words(const byte_code::ScriptPtr& script,
                                    WordCollection& collection)
        {
            // Create the structure words for the sub-scripts of this script first...
            for (const auto& sub_script : script->get_sub_scripts())
            {
                create_structure_words(sub_script, collection);
            }

            // Now create the structure words for this script.
            const auto& structures = script->get_structure_types();

            // Create the structure initialization word, and supporting accessor words.
            for (const auto& structure : structures)
            {
                // Register the structure with the word collection.
                collection.add_structure(structure);

                // Now create the structure initialization word, and supporting accessor words.
                const auto& struct_name = structure.get_name();
                const auto& struct_location = structure.get_location();

                WordInfo init_word_info;

                // Create the auto-initialization word that will automatically get called every time
                // an instance of the structure is created.  We mark these words as referenced
                // because they are always registered with the run-time by the script init code.
                init_word_info.name = structure.get_name() + ".raw-init";
                init_word_info.handler_name = filter_word_name(init_word_info.name);
                init_word_info.was_referenced = true;

                init_word_info.code = structure.get_initializer();

                // Make sure that any words used by the user init code are also marked as used.
                mark_used_words(collection, init_word_info.code.value());

                // Add the word to the collection.
                collection.add_word(std::move(init_word_info));


                // Add the user callable struct.new word now.  #.create-raw will call the registered
                // structure.raw-init word to initialize the structure.
                compilation::byte_code::Construction new_word_info(struct_location,
                                                                   struct_name + ".new");

                new_word_info.get_code().push_back(compilation::byte_code::Instruction(
                                       compilation::byte_code::Instruction::Id::push_constant_value,
                                       struct_name));
                new_word_info.get_code().push_back(compilation::byte_code::Instruction(
                                                   compilation::byte_code::Instruction::Id::execute,
                                                   "#.create-named"));

                collection.add_word(new_word_info);

                // Now create the convience structure accessor words...
                for (size_t i = 0; i < structure.get_field_names().size(); ++i)
                {
                    const auto& field_name = structure.get_field_names()[i];

                    // struct.field
                    compilation::byte_code::Construction field_index_word(
                                                                    struct_location,
                                                                    struct_name + "." + field_name);

                    field_index_word.get_code().push_back(compilation::byte_code::Instruction(
                                       compilation::byte_code::Instruction::Id::push_constant_value,
                                       static_cast<int64_t>(i)));

                    // strcut.field@
                    compilation::byte_code::Construction field_read_word(
                                                                    struct_location,
                                                                    struct_name + "." + field_name
                                                                                + "@");

                    field_read_word.get_code().push_back(compilation::byte_code::Instruction(
                                       compilation::byte_code::Instruction::Id::push_constant_value,
                                       static_cast<int64_t>(i)));
                    field_read_word.get_code().push_back(compilation::byte_code::Instruction(
                                                   compilation::byte_code::Instruction::Id::execute,
                                                   "swap"));
                    field_read_word.get_code().push_back(compilation::byte_code::Instruction(
                                                   compilation::byte_code::Instruction::Id::execute,
                                                   "#@"));

                    // struct.field@@
                    compilation::byte_code::Construction field_read_var_word(
                                                                    struct_location,
                                                                    struct_name + "." + field_name
                                                                                + "@@");
                    field_read_var_word.get_code().push_back(compilation::byte_code::Instruction(
                                       compilation::byte_code::Instruction::Id::push_constant_value,
                                       static_cast<int64_t>(i)));
                    field_read_var_word.get_code().push_back(compilation::byte_code::Instruction(
                                                   compilation::byte_code::Instruction::Id::execute,
                                                   "swap"));
                    field_read_var_word.get_code().push_back(compilation::byte_code::Instruction(
                                           compilation::byte_code::Instruction::Id::read_variable));
                    field_read_var_word.get_code().push_back(compilation::byte_code::Instruction(
                                                   compilation::byte_code::Instruction::Id::execute,
                                                   "#@"));

                    // struct.field!
                    compilation::byte_code::Construction field_write_word(
                                                                    struct_location,
                                                                    struct_name + "." + field_name
                                                                                + "!");

                    field_write_word.get_code().push_back(compilation::byte_code::Instruction(
                                       compilation::byte_code::Instruction::Id::push_constant_value,
                                       static_cast<int64_t>(i)));
                    field_write_word.get_code().push_back(compilation::byte_code::Instruction(
                                                   compilation::byte_code::Instruction::Id::execute,
                                                   "swap"));
                    field_write_word.get_code().push_back(compilation::byte_code::Instruction(
                                                   compilation::byte_code::Instruction::Id::execute,
                                                   "#!"));

                    // struct.field!!
                    compilation::byte_code::Construction field_write_var_word(
                                                                    struct_location,
                                                                    struct_name + "." + field_name
                                                                                + "!!");
                    field_write_var_word.get_code().push_back(compilation::byte_code::Instruction(
                                       compilation::byte_code::Instruction::Id::push_constant_value,
                                       static_cast<int64_t>(i)));
                    field_write_var_word.get_code().push_back(compilation::byte_code::Instruction(
                                                   compilation::byte_code::Instruction::Id::execute,
                                                   "swap"));
                    field_write_var_word.get_code().push_back(compilation::byte_code::Instruction(
                                           compilation::byte_code::Instruction::Id::read_variable));
                    field_write_var_word.get_code().push_back(compilation::byte_code::Instruction(
                                                   compilation::byte_code::Instruction::Id::execute,
                                                   "#!"));

                    collection.add_word(field_index_word);
                    collection.add_word(field_read_word);
                    collection.add_word(field_read_var_word);
                    collection.add_word(field_write_word);
                    collection.add_word(field_write_var_word);
                }
            }
        }


        // Collect all of the top-level code into one byte-code block.
        void collect_top_level_code(const byte_code::ScriptPtr& script,
                                    byte_code::ByteCode& top_level_code)
        {
            // Add the top-level code from the sub-scripts first...
            for (const auto& sub_script : script->get_sub_scripts())
            {
                collect_top_level_code(sub_script, top_level_code);
            }

            // Add the top-level code from this script.
            top_level_code.insert(top_level_code.end(),
                                  script->get_top_level().begin(),
                                  script->get_top_level().end());
        }


        // Mark all the words that are used in the top-level code.  Also mark as used any words that
        // are called by the used words.
        void mark_used_words(WordCollection& collection, const byte_code::ByteCode& code)
        {
            // Go through the top-level code and mark all the words that are used.
            for (const auto& instruction : code)
            {
                if (   (instruction.get_id() == byte_code::Instruction::Id::execute)
                    || (instruction.get_id() == byte_code::Instruction::Id::word_index))
                {
                    // Check to see if the call has been resolved.
                    if (instruction.get_value().is_int())
                    {
                        // Mark the referenced word as used.
                        auto index = static_cast<size_t>(instruction.get_value().get_int());
                        auto& word = collection.words[index];

                        if (!word.was_referenced)
                        {
                            word.was_referenced = true;

                            // If the word is a Forth word, then we need to mark all the words that
                            // it calls as referenced as well.
                            if (word.code.has_value())
                            {
                                mark_used_words(collection, word.code.value());
                            }
                        }
                    }
                    else if (instruction.get_value().is_string())
                    {
                        // Try to resolve the word name to it's index now, we'll mark it as used.
                        auto& name = instruction.get_value().get_string();
                        auto iterator = collection.word_map.find(name);

                        if (iterator != collection.word_map.end())
                        {
                            auto index = iterator->second;
                            auto& word_info = collection.words[index];

                            word_info.was_referenced = true;

                            if (word_info.code.has_value())
                            {
                                mark_used_words(collection, word_info.code.value());
                            }
                        }
                    }
                }
            }
        }


        // Create the function declaration for a word found in the collection.
        void create_word_declaration(WordInfo& word,
                                     std::shared_ptr<llvm::Module>& module,
                                     llvm::LLVMContext& context)
        {
            // Create the signature for the word's function: int8_t word_fn(void);
            llvm::FunctionType* signature = llvm::FunctionType::get(llvm::Type::getInt1Ty(context),
                                                                    false);

            // If the word is a native word, then it's external and will be linked in later.
            // Otherwise the word is a Forth word and will be internal to the module.  Be it a
            // standard library word or one from the user script(s).
            auto linkage = word.code.has_value() ? llvm::Function::InternalLinkage
                                                 : llvm::Function::ExternalLinkage;

            // Create the function signature itself.  If it's a Forth word, it's body will be filled
            // out later.
            word.function = llvm::Function::Create(signature,
                                                   linkage,
                                                   word.handler_name,
                                                   module.get());
        }


        // Create the function definitions for all of the words in the collection, both native and
        // Forth words.  But only if it was referenced by a script top-level.
        void create_word_declarations(WordCollection& collection,
                                      std::shared_ptr<llvm::Module>& module,
                                      llvm::LLVMContext& context)
        {
            for (auto& word : collection.words)
            {
                if (word.was_referenced)
                {
                    create_word_declaration(word, module, context);
                }
            }
        }


        // Create a string constant in the module.
        llvm::Value* define_string_constant(const std::string& text,
                                            llvm::IRBuilder<>& builder,
                                            std::shared_ptr<llvm::Module>& module,
                                            llvm::LLVMContext& context)
        {
            // Get our types.
            auto char_type = llvm::Type::getInt8Ty(context);
            auto char_ptr_type = llvm::PointerType::getUnqual(char_type);

            // Create the constant data array for the string then create a global variable to
            // hold it.
            auto string_constant = llvm::ConstantDataArray::getString(context, text, true);
            auto global = new llvm::GlobalVariable(*module,
                                                    string_constant->getType(),
                                                    true,
                                                    llvm::GlobalValue::PrivateLinkage,
                                                    string_constant);

            // Create a pointer to the global variable for the string constant.
            llvm::Value* string_ptr = builder.CreatePointerCast(global, char_ptr_type);

            return string_ptr;
        }


        // Generate the LLVM IR for a byte-code block.  This can be used for both Forth words and
        // the top-level script code.
        void generate_ir_for_byte_code(WordCollection& collection,
                                       const std::string& word_name,
                                       const byte_code::ByteCode& code,
                                       std::shared_ptr<llvm::Module>& module,
                                       llvm::LLVMContext& context,
                                       llvm::IRBuilder<>& builder,
                                       llvm::Function* function,
                                       GlobalMap& global_constant_map,
                                       const RuntimeApi& runtime_api,
                                       bool is_top_level)
        {
            // Gather some types we'll need.
            auto bool_type = llvm::Type::getInt1Ty(context);
            auto int64_type = llvm::Type::getInt64Ty(context);
            auto double_type = llvm::Type::getDoubleTy(context);
            auto char_type = llvm::Type::getInt1Ty(context);
            auto char_ptr_type = llvm::PointerType::getUnqual(char_type);

            // Keep track of the variables and constants that are used in the byte-code block.
            ValueMap variable_map;
            std::unordered_map<std::string, llvm::AllocaInst*> constant_map;

            size_t block_index = 1;

            // Keep track of the basic blocks we create for the jump targets.
            auto blocks = std::unordered_map<size_t, llvm::BasicBlock*>();
            auto auto_jump_blocks = std::unordered_map<size_t, std::pair<llvm::BasicBlock*,
                                                                         llvm::BasicBlock*>>();

            auto var_read_blocks = std::unordered_map<size_t, std::tuple<llvm::BasicBlock*,
                                                                         llvm::BasicBlock*,
                                                                         llvm::BasicBlock*>>();

            // Create the entry block of the function.
            auto entry_block = llvm::BasicBlock::Create(context, "entry_block", function);
            builder.SetInsertPoint(entry_block);

            // If this is the top-level code we need to register all of the script structure types
            // with the run-time.
            if (is_top_level)
            {
                for (const auto& structure : collection.structures)
                {
                    auto struct_name = define_string_constant(structure.get_name(),
                                                              builder,
                                                              module,
                                                              context);
                    auto field_count = structure.get_field_names().size();
                    auto field_count_const = llvm::ConstantInt::get(int64_type, field_count);

                    auto char_ptr_array_type = llvm::ArrayType::get(char_ptr_type, field_count);

                    auto name_array_variable = builder.CreateAlloca(char_ptr_array_type);

                    for (size_t i = 0; i < field_count; ++i)
                    {
                        auto field_name = define_string_constant(structure.get_field_names()[i],
                                                                builder,
                                                                module,
                                                                context);
                        auto array_ptr = builder.CreateStructGEP(char_ptr_array_type,
                                                                 name_array_variable,
                                                                 i);
                        builder.CreateStore(field_name, array_ptr);
                    }

                    auto iterator = collection.word_map.find(structure.get_name() +
                                                             ".raw-init");

                    if (iterator == collection.word_map.end())
                    {
                        throw std::runtime_error("Internal error, structure initializer not "
                                                 "found.");
                    }

                    auto init_handler = collection.words[iterator->second].function;

                    builder.CreateCall(runtime_api.register_structure_type,
                                       {
                                           struct_name,
                                           name_array_variable,
                                           field_count_const,
                                           init_handler
                                       });
                }
            }

            // Keep track of any loop and catch block markers.
            std::vector<std::pair<size_t, size_t>> loop_markers;
            std::vector<size_t> catch_markers;

            // Keep track of any jump targets that are the target of a catch block.
            std::set<size_t> catch_target_markers;

            // Initialize the return value to false.
            auto return_value_variable = builder.CreateAlloca(bool_type);
            builder.CreateStore(builder.getInt1(0), return_value_variable);

            // First pass...
            //
            // Gather the variables and constants that are used in the byte-code block.  We'll need
            // to generate the value variables for the variables and constants.  As well as
            // variables to hold the variable indicies at run-time.
            //
            // Also, we'll need go through the byte-code and for every instruction that can cause a
            // branch we'll need to create basic blocks for each of the possible branches.

            size_t var_index = 0;

            for (size_t i = 0; i < code.size(); ++i)
            {
                const auto& instruction = code[i];

                switch (instruction.get_id())
                {
                    case byte_code::Instruction::Id::def_variable:
                        {
                            ValueInfo info;

                            info.variable = builder.CreateAlloca(runtime_api.value_struct_type);
                            info.variable_index = builder.CreateAlloca(int64_type);
                            info.block_index = var_index;

                            ++var_index;

                            variable_map[instruction.get_value().get_string()] = info;
                        }
                        break;

                    case byte_code::Instruction::Id::def_constant:
                        if (is_top_level)
                        {
                            llvm::Constant* zero_init =
                                    llvm::ConstantAggregateZero::get(runtime_api.value_struct_type);

                            global_constant_map[instruction.get_value().get_string()] =
                                         new llvm::GlobalVariable(*module,
                                                                  runtime_api.value_struct_type,
                                                                  false,
                                                                  llvm::GlobalValue::PrivateLinkage,
                                                                  zero_init);
                        }
                        else
                        {
                            constant_map[instruction.get_value().get_string()] =
                                                builder.CreateAlloca(runtime_api.value_struct_type);

                        }
                        break;

                    case byte_code::Instruction::Id::read_variable:
                        {
                            // Create the basic blocks for the instructions.
                            std::stringstream stream;

                            stream << "block_" << block_index;
                            ++block_index;

                            auto block_a = llvm::BasicBlock::Create(context,
                                                                    stream.str(),
                                                                    function);

                            std::stringstream stream_b;

                            stream_b << "block_" << block_index;
                            ++block_index;

                            auto block_b = llvm::BasicBlock::Create(context,
                                                                    stream_b.str(),
                                                                    function);

                            var_read_blocks[i] = { block_a, block_b, nullptr };
                        }
                        break;

                    case byte_code::Instruction::Id::write_variable:
                        {
                            std::stringstream stream;

                            stream << "block_" << block_index;
                            ++block_index;

                            auto block_a = llvm::BasicBlock::Create(context,
                                                                    stream.str(),
                                                                    function);

                            std::stringstream stream_b;

                            stream_b << "block_" << block_index;
                            ++block_index;

                            auto block_b = llvm::BasicBlock::Create(context,
                                                                    stream_b.str(),
                                                                    function);

                            std::stringstream stream_c;

                            stream_c << "block_" << block_index;
                            ++block_index;

                            auto block_c = llvm::BasicBlock::Create(context,
                                                                    stream_c.str(),
                                                                    function);

                            var_read_blocks[i] = { block_a, block_b, block_c };
                        }
                        break;

                    case byte_code::Instruction::Id::execute:
                    case byte_code::Instruction::Id::jump_loop_start:
                    case byte_code::Instruction::Id::jump_loop_exit:
                    case byte_code::Instruction::Id::jump_target:
                        {
                            // Create the basic blocks for the instructions.
                            std::stringstream stream;

                            stream << "block_" << block_index;
                            ++block_index;

                            blocks[i] =
                                    llvm::BasicBlock::Create(context,
                                                             stream.str(),
                                                             builder.GetInsertBlock()->getParent());
                        }
                        break;

                    case byte_code::Instruction::Id::jump_if_zero:
                    case byte_code::Instruction::Id::jump_if_not_zero:
                        {
                            // These instructions have two jumps.  One jump for the pop error check
                            // and one for the actual jump.  So we create two blocks for each of
                            // these jump instructions.
                            std::stringstream stream;

                            stream << "block_" << block_index;
                            ++block_index;

                            auto block_a = llvm::BasicBlock::Create(context,
                                                                    stream.str(),
                                                                    function);

                            std::stringstream stream_b;

                            stream_b << "block_" << block_index;
                            ++block_index;

                            auto block_b = llvm::BasicBlock::Create(context,
                                                                    stream_b.str(),
                                                                    function);

                            auto_jump_blocks[i] = { block_a, block_b };
                        }
                        break;

                    default:
                        // Nothing to do here.
                        break;
                }
            }

            // Register the block of variables with the runtime.
            if (!variable_map.empty())
            {
                auto value_array_type = llvm::ArrayType::get(runtime_api.value_struct_ptr_type,
                                                             variable_map.size());
                auto block_array = builder.CreateAlloca(value_array_type);

                for (const auto& [_, variable] : variable_map)
                {
                    auto array_ptr = builder.CreateStructGEP(value_array_type,
                                                             block_array,
                                                             variable.block_index);
                    builder.CreateStore(variable.variable, array_ptr);
                }

                auto array_ptr = builder.CreateStructGEP(value_array_type, block_array, 0);
                auto base_index = builder.CreateCall(runtime_api.allocate_variable_block,
                                                     {
                                                        array_ptr,
                                                        builder.getInt64(variable_map.size())
                                                     });

                for (const auto& [_, variable] : variable_map)
                {
                    auto block_index = builder.getInt64(variable.block_index);
                    auto new_index = builder.CreateAdd(base_index, block_index);

                    builder.CreateStore(new_index, variable.variable_index);
                }
            }

            // Create the block to handle errors.
            auto exit_error_block = llvm::BasicBlock::Create(context, "error_block", function);

            // Create the end block of the function.
            auto exit_block = llvm::BasicBlock::Create(context, "exit_block", function);

            // Register our block of variables with the runtime.

            // Second pass...
            //
            // Now we can generate the LLVM IR for the byte-code block.
            for (size_t i = 0; i < code.size(); ++i)
            {
                const auto& instruction = code[i];

                switch (instruction.get_id())
                {
                    case byte_code::Instruction::Id::def_variable:
                        {
                            auto& variable_info
                                               = variable_map[instruction.get_value().get_string()];

                            builder.CreateCall(runtime_api.initialize_variable,
                                               { variable_info.variable });

                        }
                        break;

                    case byte_code::Instruction::Id::def_constant:
                        {
                            auto name = instruction.get_value().get_string();
                            auto iterator = constant_map.find(name);

                            llvm::Value* constant = nullptr;

                            if (iterator != constant_map.end())
                            {
                                constant = iterator->second;
                                builder.CreateCall(runtime_api.initialize_variable, { constant });
                            }
                            else
                            {
                                constant = global_constant_map[name];
                                builder.CreateCall(runtime_api.initialize_variable, { constant });
                            }

                            // Pop the value for the new constant off of the stack.
                            builder.CreateCall(runtime_api.stack_pop, { constant });
                        }
                        break;

                    case byte_code::Instruction::Id::read_variable:
                        {
                            auto index_value = builder.CreateAlloca(int64_type);
                            auto pop_result = builder.CreateCall(runtime_api.stack_pop_int,
                                                                 { index_value });

                            auto [ next_block_a, next_block_b, _ ] = var_read_blocks[i];

                            // Jump to the catch block if there is one, or the exit block if not.
                            auto error_block = catch_markers.empty()
                                                ? exit_error_block
                                                : blocks[catch_markers.back()];

                            auto cmp = builder.CreateICmpNE(pop_result, builder.getInt1(0));
                            builder.CreateCondBr(cmp, error_block, next_block_a);

                            builder.SetInsertPoint(next_block_a);

                            auto variable_temp = builder.CreateAlloca(runtime_api.value_struct_type);
                            builder.CreateCall(runtime_api.initialize_variable, { variable_temp });

                            auto index = builder.CreateLoad(int64_type, index_value);
                            auto read_result = builder.CreateCall(runtime_api.read_variable,
                                                                  { index, variable_temp });

                            cmp = builder.CreateICmpNE(read_result, builder.getInt1(0));
                            builder.CreateCondBr(cmp, error_block, next_block_b);

                            builder.SetInsertPoint(next_block_b);

                            builder.CreateCall(runtime_api.stack_push, { variable_temp });
                            builder.CreateCall(runtime_api.free_variable, { variable_temp });
                        }
                        break;

                    case byte_code::Instruction::Id::write_variable:
                        {
                            auto index_value = builder.CreateAlloca(int64_type);
                            auto pop_result = builder.CreateCall(runtime_api.stack_pop_int,
                                                                 { index_value });


                            auto [ next_block_a, next_block_b, next_block_c ] = var_read_blocks[i];

                            // Jump to the catch block if there is one, or the exit block if not.
                            auto error_block = catch_markers.empty()
                                                ? exit_error_block
                                                : blocks[catch_markers.back()];
                            auto cmp = builder.CreateICmpNE(pop_result, builder.getInt1(0));
                            builder.CreateCondBr(cmp, error_block, next_block_a);

                            builder.SetInsertPoint(next_block_a);

                            auto variable_temp =
                                                builder.CreateAlloca(runtime_api.value_struct_type);
                            builder.CreateCall(runtime_api.initialize_variable, { variable_temp });

                            pop_result = builder.CreateCall(runtime_api.stack_pop,
                                                            { variable_temp });

                            cmp = builder.CreateICmpNE(pop_result, builder.getInt1(0));
                            builder.CreateCondBr(cmp, error_block, next_block_b);

                            builder.SetInsertPoint(next_block_b);

                            auto index = builder.CreateLoad(int64_type, index_value);
                            auto write_result = builder.CreateCall(runtime_api.write_variable,
                                                                   { index, variable_temp });
                            builder.CreateCall(runtime_api.free_variable, { variable_temp });

                            cmp = builder.CreateICmpNE(write_result, builder.getInt1(0));
                            builder.CreateCondBr(cmp, error_block, next_block_c);

                            builder.SetInsertPoint(next_block_c);
                        }
                        break;

                    case byte_code::Instruction::Id::execute:
                        {
                            auto& value = instruction.get_value();

                            if (value.is_string())
                            {
                                auto name = value.get_string();

                                auto var_iter = variable_map.find(name);
                                auto const_iter = constant_map.find(name);
                                auto global_iter = global_constant_map.find(name);

                                if (var_iter != variable_map.end())
                                {
                                    auto variable_index =
                                                builder.CreateLoad(int64_type,
                                                                   var_iter->second.variable_index);
                                    builder.CreateCall(runtime_api.stack_push_int,
                                                       { variable_index });
                                }
                                else if (const_iter != constant_map.end())
                                {
                                    auto variable_temp =
                                                builder.CreateAlloca(runtime_api.value_struct_type);
                                    builder.CreateCall(runtime_api.initialize_variable,
                                                       { variable_temp });

                                    builder.CreateCall(runtime_api.deep_copy_variable,
                                                       { const_iter->second, variable_temp });

                                    builder.CreateCall(runtime_api.stack_push,
                                                       { variable_temp });

                                    builder.CreateCall(runtime_api.free_variable,
                                                       { variable_temp });
                                }
                                else if (global_iter != global_constant_map.end())
                                {
                                    auto variable_temp =
                                                builder.CreateAlloca(runtime_api.value_struct_type);
                                    builder.CreateCall(runtime_api.initialize_variable,
                                                       { variable_temp });

                                    builder.CreateCall(runtime_api.deep_copy_variable,
                                                       { global_iter->second, variable_temp });

                                    builder.CreateCall(runtime_api.stack_push,
                                                       { variable_temp });

                                    builder.CreateCall(runtime_api.free_variable,
                                                       { variable_temp });
                                }
                                else
                                {
                                    throw_error("Word " + name + " not found for execution, " +
                                                "referenced by " + word_name + ".");
                                }

                                builder.CreateBr(blocks[i]);
                                builder.SetInsertPoint(blocks[i]);
                            }
                            else if (value.is_numeric())
                            {
                                auto index = value.get_int();

                                if (   (index < 0)
                                    || (index >= collection.words.size()))
                                {
                                    throw_error("Word index " + std::to_string(index) +
                                                " out of range.");
                                }

                                auto handler = collection.words[index].function;
                                auto result = builder.CreateCall(handler, {});

                                // Check the result of the call instruction and branch to the next
                                // if no errors were raised, otherwise branch to the either the
                                // exit block or the exception handler block.
                                auto next_block = blocks[i];

                                // Jump to the catch block if there is one, or the exit block if
                                // not.
                                auto error_block = catch_markers.empty()
                                                    ? exit_error_block
                                                    : blocks[catch_markers.back()];
                                auto cmp = builder.CreateICmpNE(result, builder.getInt1(0));
                                builder.CreateCondBr(cmp, error_block, next_block);

                                // We're done with the current block, so move on to the next one.
                                builder.SetInsertPoint(next_block);
                            }
                        }
                        break;

                    case byte_code::Instruction::Id::word_index:
                        {
                            auto word_name = instruction.get_value().get_string();
                            auto exists = collection.word_map.find(word_name);

                            if (exists == collection.word_map.end())
                            {
                                throw_error("Word " + word_name + " not found for indexing.");
                            }

                            auto word_info = collection.words[exists->second];

                            auto index = exists->second;
                            auto index_const = builder.getInt64(index);

                            builder.CreateCall(runtime_api.stack_push_int, { index_const });
                        }
                        break;

                    case byte_code::Instruction::Id::word_exists:
                        {
                            auto exists = collection.word_map
                                                    .find(instruction.get_value().get_string())
                                                    != collection.word_map.end();

                            auto exists_const = llvm::ConstantInt::get(bool_type, exists);
                            builder.CreateCall(runtime_api.stack_push_bool, { exists_const });
                        }
                        break;

                    case byte_code::Instruction::Id::push_constant_value:
                        {
                            // Get the value to push from the instruction.
                            auto& value = code[i].get_value();

                            // Check the type of the value.  If it's one of the simple types
                            // directly generate the code to push it's value onto the stack.
                            if (value.is_bool())
                            {
                                auto bool_value = value.get_bool();
                                auto bool_const = llvm::ConstantInt::get(bool_type, bool_value);
                                builder.CreateCall(runtime_api.stack_push_bool,
                                                    { bool_const });
                            }
                            else if (value.is_int())
                            {
                                auto int_value = value.get_int();
                                auto int_const = llvm::ConstantInt::get(int64_type, int_value);
                                builder.CreateCall(runtime_api.stack_push_int,
                                                    { int_const });
                            }
                            else if (value.is_double())
                            {
                                auto double_value = value.get_double();
                                auto double_const = llvm::ConstantFP::get(double_type,
                                                                            double_value);
                                builder.CreateCall(runtime_api.stack_push_double,
                                                    { double_const });
                            }
                            else if (value.is_string())
                            {
                                auto string_value = value.get_string();
                                auto string_ptr = define_string_constant(string_value,
                                                                            builder,
                                                                            module,
                                                                            context);
                                builder.CreateCall(runtime_api.stack_push_string,
                                                    { string_ptr });
                            }
                            else
                            {
                                // The value is a complex type that we can't inline in the JITed
                                // code directly.  So we'll add it to the constants array and
                                // generate code to push the value from the array onto the
                                // stack.
                                /*auto index = constants.size();
                                constants.push_back(value);

                                auto index_const = builder.getInt64(index);
                                builder.CreateCall(handle_push_value_fn,
                                                    { runtime_ptr,
                                                        constant_arr_ptr,
                                                        index_const });*/
                                throw_error("TODO: Implement complex constant types.");
                            }
                        }
                        break;

                    case byte_code::Instruction::Id::mark_loop_exit:
                        {
                            // Capture the start and end indexes of the loop for later use.
                            auto start_index = i + 1;
                            auto end_index = i + code[i].get_value().get_int();

                            loop_markers.push_back({ start_index, end_index });
                        }
                        break;

                    case byte_code::Instruction::Id::unmark_loop_exit:
                        {
                            // Clear the current loop markers.
                            loop_markers.pop_back();
                        }
                        break;

                    case byte_code::Instruction::Id::mark_catch:
                        {
                            // Capture the index of the catch block for later use.
                            auto target_index = i + code[i].get_value().get_int();

                            catch_markers.push_back(target_index);
                            catch_target_markers.insert(target_index);
                        }
                        break;

                    case byte_code::Instruction::Id::unmark_catch:
                        {
                            // Clear the current catch markers so that we don't jump to then.
                            // Note that we leave the catch_target_markers set alone so that we
                            // can generate the code to push the exception onto the stack when
                            // we reach the catch target instruction.
                            catch_markers.pop_back();
                        }
                        break;

                    case byte_code::Instruction::Id::mark_context:
                        // TODO: Implement this.
                        break;

                    case byte_code::Instruction::Id::release_context:
                        // TODO: Implement this.
                        break;

                    case byte_code::Instruction::Id::jump:
                        {
                            // Jump to the target block.
                            auto index = i + code[i].get_value().get_int();
                            builder.CreateBr(blocks[index]);
                        }
                        break;

                    case byte_code::Instruction::Id::jump_if_zero:
                        {
                            // Convert the relative index to an absolute index.
                            auto index = i + code[i].get_value().get_int();

                            // Allocate a bool to hold the test value.  Then call the
                            // handle_pop_bool function to get the value from the stack.
                            auto test_value = builder.CreateAlloca(bool_type);
                            auto pop_result = builder.CreateCall(runtime_api.stack_pop_bool,
                                                                { test_value });

                            // Check the result of the call instruction and branch to the next
                            // block if no errors were raised, otherwise branch to the either
                            // the exit block or the exception handler block.
                            auto error_block = catch_markers.empty()
                                                ? exit_error_block
                                                : blocks[catch_markers.back()];

                            auto [ a, b ] = auto_jump_blocks[i];

                            auto cmp = builder.CreateICmpNE(pop_result, builder.getInt1(0));
                            builder.CreateCondBr(cmp, error_block, a);

                            // The pop was successful, so switch to the next block and generate
                            // the code to perform the jump based on the test value.
                            builder.SetInsertPoint(a);

                            // Jump to the 'success' block if the test value is true, otherwise
                            // jump to the 'fail' block.
                            auto read_value = builder.CreateLoad(bool_type, test_value);

                            builder.CreateCondBr(read_value, b, blocks[index]);
                            builder.SetInsertPoint(b);
                        }
                        break;

                    case byte_code::Instruction::Id::jump_if_not_zero:
                        {
                            // Convert the relative index to an absolute index.
                            auto index = i + code[i].get_value().get_int();

                            // Allocate a bool to hold the test value.  Then call the
                            // handle_pop_bool function to get the value from the stack.
                            auto test_value = builder.CreateAlloca(bool_type);
                            auto pop_result = builder.CreateCall(runtime_api.stack_pop_bool,
                                                                { test_value });

                            // Check the result of the call instruction and branch to the next
                            // block if no errors were raised, otherwise branch to the either
                            // the exit block or the exception handler block.
                            auto error_block = catch_markers.empty()
                                                ? exit_error_block
                                                : blocks[catch_markers.back()];

                            auto [ a, b ] = auto_jump_blocks[i];

                            auto cmp = builder.CreateICmpNE(pop_result, builder.getInt1(0));
                            builder.CreateCondBr(cmp, error_block, a);

                            // The pop was successful, so switch to the next block and generate
                            // the code to perform the jump based on the test value.
                            builder.SetInsertPoint(a);

                            // Jump to the 'success' block if the test value is true, otherwise
                            // jump to the 'fail' block.
                            auto read_value = builder.CreateLoad(bool_type, test_value);
                            builder.CreateCondBr(read_value, blocks[index], b);
                            builder.SetInsertPoint(b);
                        }
                        break;

                    case byte_code::Instruction::Id::jump_loop_start:
                        {
                            // Jump to the start block of the loop.
                            auto start_index = loop_markers.back().first;

                            builder.CreateBr(blocks[start_index]);
                            builder.SetInsertPoint(blocks[i]);
                        }
                        break;

                    case byte_code::Instruction::Id::jump_loop_exit:
                        {
                            // Jump to the end block of the loop.
                            auto end_index = loop_markers.back().second;
                            builder.CreateBr(blocks[end_index]);
                            builder.SetInsertPoint(blocks[i]);
                        }
                        break;

                    case byte_code::Instruction::Id::jump_target:
                        // Make sure that the current block has a terminator instruction, if
                        // not, add one to jump to the next block.  This is because this
                        // would be a natural follow through in the original byte-code.
                        if (builder.GetInsertBlock()->getTerminator() == nullptr)
                        {
                            builder.CreateBr(blocks[i]);
                        }

                        // We're done with the current block, switch over to the next.
                        builder.SetInsertPoint(blocks[i]);


                        // If this is a catch target, we need to generate the code to push
                        // the exception onto the stack and clear the last exception.
                        if (   (!catch_target_markers.empty())
                            && (catch_target_markers.find(i) != catch_target_markers.end()))
                        {
                            auto last_error = builder.CreateCall(runtime_api.get_last_error, {});

                            builder.CreateCall(runtime_api.stack_push_string, { last_error });
                            builder.CreateCall(runtime_api.clear_last_error, {});
                        }
                        break;
                }
            }


            // Make sure that the last block has a terminator instruction, if not, add one to
            // jump to the exit block.
            if (builder.GetInsertBlock()->getTerminator() == nullptr)
            {
                builder.CreateBr(exit_block);
            }

            // If an error occurs set the return value to true.
            builder.SetInsertPoint(exit_error_block);
            builder.CreateStore(builder.getInt1(1), return_value_variable);
            builder.CreateBr(exit_block);


            // Make sure that all the auto-jump blocks have a terminator instruction.
            for (auto block : blocks)
            {
                if (block.second->getTerminator() == nullptr)
                {
                    builder.SetInsertPoint(block.second);
                    builder.CreateBr(exit_block);
                }
            }

            // We're done with the last user code block, so switch over to the exit block and
            // generate the code to return from the function.
            builder.SetInsertPoint(exit_block);

            // Check to see if we needed to allocated a block of variables...
            if (!variable_map.empty())
            {
                // Release the variable block from the runtime.
                builder.CreateCall(runtime_api.release_variable_block, {});
            }

            // First off, free all local variables and constants.
            for (auto iterator = variable_map.begin(); iterator != variable_map.end(); ++iterator)
            {
                builder.CreateCall(runtime_api.free_variable, { iterator->second.variable });
            }

            if (is_top_level)
            {
                for (auto iterator = global_constant_map.begin();
                     iterator != global_constant_map.end();
                     ++iterator)
                {
                    builder.CreateCall(runtime_api.free_variable, { iterator->second });
                }
            }
            else
            {
                for (auto iterator = constant_map.begin();
                     iterator != constant_map.end();
                     ++iterator)
                {
                    builder.CreateCall(runtime_api.free_variable, { iterator->second });
                }
            }

            // Return and pass the return value.
            auto return_value = builder.CreateLoad(bool_type, return_value_variable);
            builder.CreateRet(return_value);
        }


        // Go through the collection of words and compile the ones that were referenced.
        void compile_used_words(WordCollection& collection,
                                std::shared_ptr<llvm::Module>& module,
                                llvm::LLVMContext& context,
                                llvm::IRBuilder<>& builder,
                                const RuntimeApi& runtime_api,
                                GlobalMap& global_constant_map)
        {
            for (const auto& word : collection.words)
            {
                // If the word was referenced and is a Forth word...
                if (   (word.was_referenced)
                    && (word.code.has_value()))
                {
                    // Create the word's IR function body.
                    generate_ir_for_byte_code(collection,
                                              word.name,
                                              word.code.value(),
                                              module,
                                              context,
                                              builder,
                                              word.function,
                                              global_constant_map,
                                              runtime_api,
                                              false);
                }
            }
        }


        void compile_top_level_code(WordCollection& collection,
                                    const byte_code::ByteCode& top_level_code,
                                    std::shared_ptr<llvm::Module>& module,
                                    llvm::LLVMContext& context,
                                    llvm::IRBuilder<>& builder,
                                    const RuntimeApi& runtime_api,
                                    GlobalMap& global_constant_map)
        {
            // Start off by creating the signature for the top-level function.
            llvm::FunctionType* top_level_signature =
                                            llvm::FunctionType::get(llvm::Type::getInt1Ty(context),
                                                                    false);

            // Create the function itself.
            llvm::Function* top_level_function =
                                             llvm::Function::Create(top_level_signature,
                                                                    llvm::Function::ExternalLinkage,
                                                                    "script_top_level",
                                                                    module.get());

            generate_ir_for_byte_code(collection,
                                      "script_top_level",
                                      top_level_code,
                                      module,
                                      context,
                                      builder,
                                      top_level_function,
                                      global_constant_map,
                                      runtime_api,
                                      true);
        }


        // Create the word table for the runtime.
        void create_word_table(const WordCollection& collection,
                               std::shared_ptr<llvm::Module>& module,
                               llvm::LLVMContext& context)
        {
            auto function_type = llvm::FunctionType::get(llvm::Type::getInt1Ty(context), false);
            auto function_ptr_type = llvm::PointerType::getUnqual(function_type);
            auto table_size = collection.words.size();

            auto word_table_type = llvm::ArrayType::get(function_ptr_type, table_size);
            std::vector<llvm::Constant*> word_table_values;

            for (size_t i = 0; i < table_size; ++i)
            {
                auto& word = collection.words[i];

                if (word.was_referenced)
                {
                    word_table_values.push_back(word.function);
                }
                else
                {
                    word_table_values.push_back(llvm::Constant::getNullValue(function_ptr_type));
                }
            }

            auto word_table_constant = llvm::ConstantArray::get(word_table_type, word_table_values);

            auto word_table = new llvm::GlobalVariable(*module,
                                                       word_table_type,
                                                       true,
                                                       llvm::GlobalValue::ExternalLinkage,
                                                       word_table_constant,
                                                       "word_table");
        }


        void optimize_module(const std::shared_ptr<llvm::Module>& module)
        {
            // Create the pass manager that will run the optimization passes on the module.
            llvm::PassBuilder pass_builder;
            llvm::LoopAnalysisManager loop_am;
            llvm::FunctionAnalysisManager function_am;
            llvm::CGSCCAnalysisManager cgsccam;
            llvm::ModuleAnalysisManager module_am;

            pass_builder.registerModuleAnalyses(module_am);
            pass_builder.registerCGSCCAnalyses(cgsccam);
            pass_builder.registerFunctionAnalyses(function_am);
            pass_builder.registerLoopAnalyses(loop_am);
            pass_builder.crossRegisterProxies(loop_am, function_am, cgsccam, module_am);

            llvm::ModulePassManager mpm =
                            pass_builder.buildPerModuleDefaultPipeline(llvm::OptimizationLevel::O3);

            // Now, run the optimization passes on the module.
            mpm.run(*module, module_am);
        }


    }


    // Generate the LLVM IR for a script and it's sub-scripts, and write the resulting IR to an
    // object file.
    void generate_llvm_ir(const byte_code::ScriptPtr& standard_library,
                          const byte_code::ScriptPtr& script,
                          const std::filesystem::path& output_path)
    {
        // Create the LLVM context for the compilation, then create the module that will hold the
        // generated LLVM IR.
        llvm::LLVMContext context;

        auto module = std::make_shared<llvm::Module>(script->get_script_path().string(), context);
        llvm::IRBuilder<> builder(context);

        // Register the runtime-library's API with the module.
        auto runtime_api = register_runtime_api(module);

        // Gather all the words from the runtime, the standard library, and the script and it's
        // sub-scripts.
        WordCollection words;

        gather_runtime_words(words);
        gather_script_words(standard_library, words);
        gather_script_words(script, words);

        // Create the structure words for the runtime and the standard library.  These words will
        // be used to init and access the script's data structures.
        create_structure_words(standard_library, words);
        create_structure_words(script, words);

        // Now that all words have been gathered, we can try to resolve all calls one last time.
        // After this the only unresolved calls should be variable and constant accesses which will
        // get resolved later.  Any non-variable or constant calls that are unresolved at this point
        // are a fatal error, which we'll check for during the code generation phase.
        try_resolve_words(words);

        // Build up the full top-level code for the script, including any sub-scripts and the
        // standard library.
        byte_code::ByteCode top_level_code;
        GlobalMap const_map;

        collect_top_level_code(standard_library, top_level_code);
        collect_top_level_code(script, top_level_code);

        // Try to resolve all the calls in the top-level code.
        try_resolve_calls(words, top_level_code);

        // Now that we have all the top-levels collected, we can go through that code and mark
        // words as used or unused.  Then make sure that all of the used words have been properly
        // declared.
        mark_used_words(words, top_level_code);
        create_word_declarations(words, module, context);

        // Create the script top-level function.  This function will be the entry point for the
        // resulting program.  It'll be made of of all the top level blocks in each of the scripts
        // that we gathered previously.
        compile_top_level_code(words,
                               top_level_code,
                               module,
                               context,
                               builder,
                               runtime_api,
                               const_map);

        // Compile the function bodies for all used Forth words.
        compile_used_words(words, module, context, builder, runtime_api, const_map);


        // Create the word_table for the runtime.
        create_word_table(words, module, context);


        // Now that all code has been generated, verify that the module is well-formed.
        if (verifyModule(*module, &llvm::errs()))
        {
            throw_error("Generated LLVM IR module is invalid.");
        }

        // Apply LLVM optimization passes to the module.
        optimize_module(module);

        // We've generated our code and optimized it, we can now write the LLVM IR to an object
        // file.

        // Get the target triple for the host machine.
        auto target_triple = llvm::sys::getDefaultTargetTriple();

        // Find the llvm target for the host machine.
        std::string error;
        const auto target = llvm::TargetRegistry::lookupTarget(target_triple, error);

        if (!target)
        {
            throw_error("Failed to lookup LLVM target: " + error);
        }

        // Set the module's target triple and data layout.
        llvm::TargetOptions options;

        auto reloc_model = std::optional<llvm::Reloc::Model>(llvm::Reloc::PIC_);
        auto target_machine = target->createTargetMachine(target_triple,
                                                          "generic",
                                                          "",
                                                          options,
                                                          reloc_model);

        module->setTargetTriple(target_triple);
        module->setDataLayout(target_machine->createDataLayout());

        // Uncomment the following line to print the module to stdout for debugging.
        // module->print(llvm::outs(), nullptr);

        // Write the module to an object file while compiling it to native code.
        std::error_code error_code;
        llvm::raw_fd_ostream output_stream(output_path.string(), error_code);

        if (error_code)
        {
            throw_error("Failed to open output file: " + error_code.message());
        }

        llvm::legacy::PassManager pass_manager;

        if (target_machine->addPassesToEmitFile(pass_manager,
                                                output_stream,
                                                nullptr,
                                                llvm::CodeGenFileType::ObjectFile))
        {
            throw_error("Failed to setup target machine to emit object file.");
        }

        pass_manager.run(*module);
        output_stream.flush();
    }


}

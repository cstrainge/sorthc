
#pragma once



namespace sorthc::compilation::byte_code
{


    // Represents a byte-code instruction in the Strange Forth virtual machine.  All Strange Forth
    // words are compiled into a sequence of these instructions.  Once generated a second pass
    // converts these instructions into LLVM instructions for further optimization and compilation
    // into native code.
    class Instruction
    {
        public:
            enum class Id : unsigned char
            {
                def_variable,
                def_constant,
                read_variable,
                write_variable,
                execute,
                word_index,
                word_exists,
                push_constant_value,
                mark_loop_exit,
                unmark_loop_exit,
                mark_catch,
                unmark_catch,
                mark_context,
                release_context,
                jump,
                jump_if_zero,
                jump_if_not_zero,
                jump_loop_start,
                jump_loop_exit,
                jump_target
            };

        private:
            std::optional<source::Location> location;
            Id id;
            run_time::Value value;

        public:
            Instruction() noexcept;
            Instruction(const source::Location& location,
                        Id id,
                        const run_time::Value& value) noexcept;
            Instruction(Id id, const run_time::Value& value) noexcept;
            Instruction(Id id) noexcept;
            Instruction(const Instruction& other) noexcept = default;
            Instruction(Instruction&& other) noexcept = default;
            ~Instruction() noexcept = default;

        public:
            Instruction& operator=(const Instruction& other) noexcept = default;
            Instruction& operator=(Instruction&& other) noexcept = default;

        public:
            const std::optional<source::Location>& get_location() const noexcept;
            Id get_id() const noexcept;
            const run_time::Value& get_value() const noexcept;
    };


    using ByteCode = std::vector<Instruction>;


}

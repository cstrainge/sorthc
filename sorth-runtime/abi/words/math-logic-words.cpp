
#include "sorth-runtime.h"
#include "math-logic-words.h"



using namespace sorth::run_time::data_structures;


extern "C"
{


        namespace
        {
            uint8_t string_or_numeric_op(std::function<void(double, double)> dop,
                                         std::function<void(int64_t, int64_t)> iop,
                                         std::function<void(std::string, std::string)> sop)
            {
                Value b;
                Value a;

                auto pop_result_1 = stack_pop(&b);
                auto pop_result_2 = stack_pop(&a);

                if (pop_result_1 || pop_result_2)
                {
                    return 1;
                }

                if (Value::either_is_string(a, b))
                {
                    sop(a.get_string(), b.get_string());
                }
                else if (Value::either_is_float(a, b))
                {
                    dop(a.get_double(), b.get_double());
                }
                else if (Value::either_is_numeric(a, b))
                {
                    iop(a.get_int(), b.get_int());
                }
                else
                {
                    set_last_error("Expected string or numeric values.");
                    return 1;
                }

                return 0;
            }


            uint8_t math_op(std::function<double(double, double)> dop,
                            std::function<int64_t(int64_t, int64_t)> iop)
            {
                Value b;
                Value a;

                auto pop_result_1 = stack_pop(&b);
                auto pop_result_2 = stack_pop(&a);

                if (pop_result_1 || pop_result_2)
                {
                    return 1;
                }

                Value result;

                if (Value::either_is_float(a, b))
                {
                    result = dop(a.get_double(), b.get_double());
                }
                else if (Value::either_is_integer(a, b))
                {
                    result = iop(a.get_int(), b.get_int());
                }
                else
                {
                    set_last_error("Expected numeric values.");
                    return 1;
                }

                stack_push(&result);
                return 0;
            }


            uint8_t logic_op(std::function<bool(bool, bool)> op)
            {
                bool b;
                bool a;

                auto pop_result_1 = stack_pop_bool(&b);
                auto pop_result_2 = stack_pop_bool(&a);

                if (pop_result_1 || pop_result_2)
                {
                    return 1;
                }

                auto result = op(a, b);

                stack_push_bool(result);

                return 0;
            }


            uint8_t logic_bit_op(std::function<int64_t(int64_t, int64_t)> op)
            {
                int64_t b;
                int64_t a;

                auto pop_result_1 = stack_pop_int(&b);
                auto pop_result_2 = stack_pop_int(&a);

                if (pop_result_1 || pop_result_2)
                {
                    return 1;
                }

                auto result = op(a, b);

                stack_push_int(result);

                return 0;
            }
        }


        uint8_t word_add()
        {
            return string_or_numeric_op([&](auto a, auto b) { stack_push_double(a + b); },
                                        [&](auto a, auto b) { stack_push_int(a + b); },
                                        [&](auto a, auto b) { stack_push_string((a + b).c_str()); });
        }


        uint8_t word_subtract()
        {
            return math_op([](auto a, auto b) -> auto { return a - b; },
                           [](auto a, auto b) -> auto { return a - b; });
        }


        uint8_t word_multiply()
        {
            return math_op([](auto a, auto b) -> auto { return a * b; },
                           [](auto a, auto b) -> auto { return a * b; });
        }


        uint8_t word_divide()
        {
            return math_op([](auto a, auto b) -> auto { return a / b; },
                           [](auto a, auto b) -> auto { return a / b; });
        }


        uint8_t word_mod()
        {
            int64_t b;
            int64_t a;

            auto pop_result_1 = stack_pop_int(&b);
            auto pop_result_2 = stack_pop_int(&a);

            if (pop_result_1 || pop_result_2)
            {
                return 1;
            }

            stack_push_int(a % b);

            return 0;
        }


        uint8_t word_logic_and()
        {
            logic_op([](auto a, auto b) { return a && b; });

            return 0;
        }


        uint8_t word_logic_or()
        {
            return logic_op([](auto a, auto b) { return a || b; });
        }


        uint8_t word_logic_not()
        {
            bool value;

            auto pop_result = stack_pop_bool(&value);

            if (pop_result)
            {
                return 1;
            }

            stack_push_bool(!value);

            return 0;
        }


        uint8_t word_bit_and()
        {
            return logic_bit_op([](auto a, auto b) { return a & b; });
        }


        uint8_t word_bit_or()
        {
            return logic_bit_op([](auto a, auto b) { return a | b; });
        }


        uint8_t word_bit_xor()
        {
            return logic_bit_op([](auto a, auto b) { return a ^ b; });
        }


        uint8_t word_bit_not()
        {
            int64_t value;

            auto pop_result = stack_pop_int(&value);

            if (pop_result)
            {
                return 1;
            }

            stack_push_int(~value);

            return 0;
        }


        uint8_t word_bit_left_shift()
        {
            return logic_bit_op([](auto value, auto amount) { return value << amount; });
        }


        uint8_t word_bit_right_shift()
        {
            return logic_bit_op([](auto value, auto amount) { return value >> amount; });
        }


        uint8_t word_equal()
        {
            return string_or_numeric_op([&](auto a, auto b) { stack_push_bool(a == b); },
                                        [&](auto a, auto b) { stack_push_bool(a == b); },
                                        [&](auto a, auto b) { stack_push_bool(a == b); });
        }


        uint8_t word_greater_equal()
        {
            return string_or_numeric_op([&](auto a, auto b) { stack_push_bool(a >= b); },
                                        [&](auto a, auto b) { stack_push_bool(a >= b); },
                                        [&](auto a, auto b) { stack_push_bool(a >= b); });
        }


        uint8_t word_less_equal()
        {
            return string_or_numeric_op([&](auto a, auto b) { stack_push_bool(a <= b); },
                                        [&](auto a, auto b) { stack_push_bool(a <= b); },
                                        [&](auto a, auto b) { stack_push_bool(a <= b); });
        }


        uint8_t word_greater()
        {
            return string_or_numeric_op([&](auto a, auto b) { stack_push_bool(a > b); },
                                        [&](auto a, auto b) { stack_push_bool(a > b); },
                                        [&](auto a, auto b) { stack_push_bool(a > b); });
        }


        uint8_t word_less()
        {
            return string_or_numeric_op([&](auto a, auto b) { stack_push_bool(a < b); },
                                        [&](auto a, auto b) { stack_push_bool(a < b); },
                                        [&](auto a, auto b) { stack_push_bool(a < b); });
        }


}


namespace sorth::run_time::abi::words
{


    void register_math_logic_words(const RuntimeWordRegistrar& registrar)
    {
        // Math ops.
        registrar("+", "word_add");
        registrar("-", "word_subtract");
        registrar("*", "word_multiply");
        registrar("/", "word_divide");
        registrar("%", "word_mod");

        // Logical words.
        registrar("&&", "word_logic_and");
        registrar("||", "word_logic_or");
        registrar("'", "word_logic_not");

        // Bitwise operator words.
        registrar("&", "word_bit_and");
        registrar("|", "word_bit_or");
        registrar("^", "word_bit_xor");
        registrar("~", "word_bit_not");
        registrar("<<", "word_bit_left_shift");
        registrar(">>", "word_bit_right_shift");

        // Equality words.
        registrar("=", "word_equal");
        registrar(">=", "word_greater_equal");
        registrar("<=", "word_less_equal");
        registrar(">", "word_greater");
        registrar("<", "word_less");
    }


}

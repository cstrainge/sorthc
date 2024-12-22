
: variable immediate description: "Define a new variable."
                     signature: "variable <new_name>"
    word op.def_variable
;

: @ immediate description: "Read from a variable index."
              signature: "variable -- value"
    op.read_variable
;

: ! immediate  description: "Write to a variable at the given index."
               signature: "value variable -- "
    op.write_variable
;

: variable! immediate description: "Define a new variable with a default value."
                      signature: "new_value variable! <new_name>"
    word dup

    op.def_variable
    op.execute
    op.write_variable
;



: constant immediate description: "Define a new constant value."
                     signature: "new_value constant <new_name>"
    word op.def_constant
;



: sentinel_word hidden
    "The word " swap + " should not be run directly outside of it's syntax." + throw
;



: mark_context immediate description: "Create a new variable and word context."
                         signature: " -- "
    op.mark_context
;

: release_context immediate description: "Release the current context freeing it's variables and words."
                            signature: " -- "
    op.release_context
;



: if immediate
    unique_str variable! else_label
    unique_str variable! end_label

    code.new_block

    else_label @ op.jump_if_zero
    "else" 1 code.compile_until_words
    drop

    end_label @ op.jump

    else_label @ op.jump_target

    "then" 1 code.compile_until_words
    drop

    end_label @ op.jump_target

    code.resolve_jumps
    code.merge_stack_block
;

: if immediate description: "Definition of the if else then syntax."
               signature: "<test> if <code> [else <code>] then"
    unique_str variable! if_fail_label

    code.new_block

    if_fail_label @ op.jump_if_zero

    "else" "then" 2 code.compile_until_words

    "then" =
    if
        if_fail_label @ op.jump_target
    else
        unique_str variable! then_label

        then_label @ op.jump

        if_fail_label @ op.jump_target

        "then" 1 code.compile_until_words
        drop

        then_label @ op.jump_target
    then

    code.resolve_jumps
    code.merge_stack_block
;

: else immediate description: "Define an else clause for an if statement."
    "else" sentinel_word
;

: then immediate description: "End of an if/else/then block."
    "then" sentinel_word
;

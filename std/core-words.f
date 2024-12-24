
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



: begin immediate description: "Defines loop until and loop repeat syntaxes."
                  signature: "begin <code> <test> until *or* begin <test> while <code> repeat"
    unique_str variable! top_label
    unique_str variable! end_label

    code.new_block

    end_label @ op.mark_loop_exit
    top_label @ op.jump_target

    "while" "until" 2 code.compile_until_words

    "until" =
    if
        top_label @ op.jump_if_zero
        end_label @ op.jump_target
        op.unmark_loop_exit
    else
        end_label @ op.jump_if_zero

        "repeat" 1 code.compile_until_words
        drop

        top_label @ op.jump
        end_label @ op.jump_target
        op.unmark_loop_exit
    then

    code.resolve_jumps
    code.merge_stack_block
;



: until description: "The end of a loop/until block."
    "until" sentinel_word
;



: while description: "Part of a begin/while/repeat block."
    "while" sentinel_word
;



: repeat description: "The end of a begin/while/repeat block."
    "repeat" sentinel_word
;



: break immediate description: "Break out of the current loop."
    op.jump_loop_exit
;


: continue immediate description: "Immediately jump to the next iteration of the loop."
    op.jump_loop_start
;



: ( immediate description: "Defines comment syntax."
    begin
        word ")" =
    until
;



: ) immediate description: "The end of a comment block."
    ")" sentinel_word
;



( Now that we've defined comments, we can begin to document the code.  So far we've defined a few  )
( base words.  Their implementations are to simply generate an instruction that will perform their )
( function into the bytecode stream being generated. )

( Next we define the 'if' statement.  The first one is just a basic version where else blocks are  )
( mandatory.  Right after that we redefine 'if' to have a more flexible implementation.  Note that )
( we use the previous definition to assist us in creating the new one. )

( Building on that we define the two forms of the 'begin' loop. 'begin until' and 'begin repeat'.  )
( Once we had those things in place, we were able to define the comment block. )


( Case statement of the form:                                                                      )
(                                                                                                  )
(     case                                                                                         )
(         <test> of                                                                                )
(             <body>                                                                               )
(             endof                                                                                )
(                                                                                                  )
(         <test> of                                                                                )
(             <body>                                                                               )
(             endof                                                                                )
(         ...                                                                                      )
(                                                                                                  )
(         <default body>                                                                           )
(     endcase                                                                                      )
(                                                                                                  )
( Where it's expected to have an input value left on the stack, and each test block generates a    )
( value that's compared against that input for equality. )

: case immediate description: "Defines case/of/endcase syntax."
                 signature: "<value> case <value> of <code> endof ... <code> endcase"
    false variable! done

    ( Label marking end of the entire case statement. )
    unique_str variable! case_end_label

    ( Label for the next of statement test or the beginning of the default block. )
    unique_str variable! next_label

    ( We create 2 code blocks on the construction stack.  The top one will hold the current case )
    ( or default block.  The bottom one is where we will consolidate everything for final        )
    ( resolution back into the base block we are generating for. )
    code.new_block
    code.new_block

    begin
        ( Ok, compile either the case of test or the end case block.  We won't know for sure which )
        ( it is until we hit the next keyword. )
        "of" "endcase" 2 code.compile_until_words

        "of" =
        if
            ( We've just compiled the case of test.  We need to duplicate and preserve the input )
            ( value before the test code burns it up.  So, insert the call to dup at the         )
            ( beginning of the current code block. )
            true code.insert_at_front
            "dup" op.execute
            false code.insert_at_front

            ( Now, check to see if the value the case test left on the stack is equal to the input )
            ( we were given before the case statement began executing. )
            "=" op.execute

            ( If the test fails, jump to the next case test.  If it succeeds we drop the input )
            ( value as it isn't needed anymore. )
            next_label @ op.jump_if_zero
            "drop" op.execute

            ( Compile the body of the case block itself. )
            "endof" 1 code.compile_until_words
            drop

            ( Once the block is done executing, jump to the end of the whole statement. )
            case_end_label @ op.jump

            ( Now that we're outside of the case block, we can mark the beginning of the next one. )
            ( Note that we also generate a new unique id for the next case block, should we find   )
            ( one. )
            next_label @ op.jump_target
            unique_str next_label !

            ( Merge this block into the base one, and create a new one for the next section we )
            ( find. )
            code.merge_stack_block
            code.new_block
        else
            ( Looks like we've found the default case block.  Again, we need to insert an          )
            ( instruction before the user code.  In this case it's to drop the input value as it's )
            ( not needed anymore. )
            true code.insert_at_front
            "drop" op.execute
            false code.insert_at_front

            ( We can now mark a jump target for the end of the entire case statement.  We also )
            ( note that we are done with the loop here. )
            case_end_label @ op.jump_target
            true done !

            ( Merge the last block into the base code. )
            code.merge_stack_block
        then

        ( A simple loop until done loop. )
        done @
    until

    ( Ok, resolve all of the jump symbols and merge this block back into the base code being )
    ( compiled by the interpreter. )
    code.resolve_jumps
    code.merge_stack_block
;



: of immediate description: "Defines a test clause of a case block."
    "of" sentinel_word
;



: endof immediate description: "Ends a clause of a case block."
    "endof" sentinel_word
;



: endcase immediate description: "End of a case block."
    "endcase" sentinel_word
;



: do immediate description: "Define a do loop syntax."
               signature: "start_value end_value do <loop-body> loop"
    ( Keep track of the start and end labels for the loop. )
    unique_str variable! top_label
    unique_str variable! end_label

    ( Create a new sub-block of instructions for this loop. )
    code.new_block

    ( Create variables to track the end boundary and loop index. )
    unique_str variable! end_value
    unique_str variable! index


    ( Get the end value off the top of the stack and store in a constant it for comparison. )
    end_value @  op.def_constant


    ( Define the loop index starting at the next value on the stack. )
    index @ dup  op.def_variable
                 op.execute
    op.write_variable

    ( Mark the beginning of the loop. )
    end_label @ op.mark_loop_exit
    top_label @ op.jump_target

    ( Generate the loop comparison. )
    index @ op.execute
    op.read_variable

    end_value @ op.execute

    "<" op.execute
    end_label @ op.jump_if_zero


    ( Compile the loop body. )
    "loop" 1 code.compile_until_words
    drop


    ( Compile the increment and loop repeat. )
    index @ op.execute
    "++!" op.execute
    top_label @ op.jump

    ( Mark the end of the loop. )
    end_label @ op.jump_target
    op.unmark_loop_exit


    ( Clean up and merge the new code. )
    code.resolve_jumps
    code.merge_stack_block
;



: loop description: "The end of a do loop."
    "loop" sentinel_word
;



: # immediate description: "Beginning of a structure definition."
              signature: "# name field_name [ -> default_value ] ... ;"
    word variable! struct_name
    false variable! is_hidden
    variable field_name

    0 [].new variable! fields
    0 variable! index

    false variable! found_initializers

    code.new_block

    begin
        true
    while
        word field_name !

        field_name @
        case
            "hidden" of
                    true is_hidden !
                    continue
                endof

            "(" of
                    "(" execute
                    continue
                endof

            "->" of
                    true found_initializers !

                    "dup" op.execute
                    ";" "," 2 code.compile_until_words

                    "swap" op.execute
                    index @ -- op.push_constant_value
                    "swap" op.execute
                    "[]!" op.execute

                    ";" =
                    if
                        break
                    then
                endof

            ";" of
                    break
                endof

            index @ 1 + fields [].size!!

            field_name @ fields [ index @ ]!!

            index @ 1 + index !
        endcase
    repeat

    struct_name @        op.push_constant_value
    fields @             op.push_constant_value
    is_hidden @          op.push_constant_value
    found_initializers @ op.push_constant_value

    ( If we found any initializer code, pop it off the construction stack onto the main stack for )
    ( the base structure definition word to use. )
    found_initializers @
    if
        code.pop_stack_block
    else
        code.drop_stack_block
    then

    "sorth.define_struct" op.execute
;



( Some useful stack management words. )
: nip description: "Nip the second from the top item from the stack."
      signature: "a b c -- a c"
    swap
    drop
;



: 2drop description: "Drop the top two items from the stack."
        signature: "a b -- "
    drop
    drop
;



: 3drop description: "Drop the top three items from the stack."
        signature: "a b -- "
    3 x-drop
;



: 4drop description: "Drop the top two items from the stack."
        signature: "a b -- "
    4 x-drop
;



: x-drop description: "Drop the top n items from the stack."
        signature: "count -- "
    variable! count

    begin
        count @  0  >
    while
        drop
        count --!
    repeat
;



: []!! description: "Write a value at an index to the array variable."
       signature: "new_value index array_variable -- "
    @ []!
;



: []@@ description: "Read a value from an index from the array variable."
       signature: "index array_variable -- value"
    @ []@
;



: [].size@@ description: "Read the array variable's current size."
            signature: "array_variable -- size"
    @ [].size@
;



: [].size!! description: "Shrink or grow the array variable to the given size."
            signature: "new_size array_variable -- "
    @ [].size!
;



: [].size++!  description: "Grow an array by one item."
              signature: "array -- "
    variable! the_array

    the_array [].size@@ ++ the_array [].size!!
;



: [].size++!!  description: "Grow an array variable by one item."
               signature: "array_variable -- "
    @ variable! the_array

    the_array [].size@@ ++ the_array [].size!!
;



: [].size--!!  description: "Shrink an array variable by one item."
               signature: "array_variable -- "
    @ variable! the_array

    the_array [].size@@ -- the_array [].size!!
;



: [].push_front!! description: "Push a new value to the top of an array variable."
                  signature: "value array_variable -- "
    @ [].push_front!
;



: [].push_back!! description: "Push a new value to the end of an array variable."
                  signature: "value array_variable -- "
    @ [].push_back!
;



: [].pop_front!! description: "Pop a value from the top of an array variable."
                  signature: "array_variable -- value"
    @ [].pop_front!
;



: [].pop_back!! description: "Pop a value from the bottom of an array variable."
                  signature: "array_variable -- value"
    @ [].pop_back!
;



: [ immediate
    description: "Define 'array [ index or indices ]' access or `[ value , ... ]` creation."
    signature: "array [ <index> ]<operation> *or* [ <value_list> ]"

    1 variable! index_count
    1 [].new variable! index_blocks

    variable command

    false variable! found_end_bracket
    true variable! is_writing
    false variable! is_creating

    code.new_block

    begin
        "," "]!" "]!!" "]@" "]@@" "]" 6 code.compile_until_words

        code.pop_stack_block index_count @ 1 - index_blocks []!!

        case
            "," of
                index_count @ 1 +  index_count !

                code.new_block
                index_count @ index_blocks [].size!!
            endof

            "]"   of  "[].new" command !    true found_end_bracket !  true is_creating !  endof
            "]!"  of  "[]!"    command !    true found_end_bracket !                      endof
            "]!!" of  "[]!!"   command !    true found_end_bracket !                      endof
            "]@"  of  "[]@"    command !    true found_end_bracket !  false is_writing !  endof
            "]@@" of  "[]@@"   command !    true found_end_bracket !  false is_writing !  endof
        endcase

        found_end_bracket @
    until

    is_creating @
    if
        index_count @ op.push_constant_value
        command @ op.execute

        0 index_count !

        begin
            index_count @ index_blocks [].size@@ <
        while
            index_count @ index_blocks []@@ code.push_stack_block

            code.stack-block-size@ 0>
            if
                true code.insert_at_front
                "dup" op.execute
                false code.insert_at_front

                "swap" op.execute
                index_count @ op.push_constant_value
                "swap" op.execute
                "[]!" op.execute
            then

            code.merge_stack_block
            index_count @ 1 + index_count !
        repeat
    else
        index_count @ 1 =
        if
            0 index_blocks []@@ code.push_stack_block

            "swap" op.execute
            command @ op.execute

            code.merge_stack_block
        else
            index_count @ 1 - variable! i

            begin
                i @ index_blocks []@@ code.push_stack_block

                is_writing @
                if
                    true code.insert_at_front
                    "over" op.execute
                    false code.insert_at_front
                else
                    true code.insert_at_front
                    "dup" op.execute
                    false code.insert_at_front
                then

                "swap" op.execute
                command @ op.execute

                is_writing @ true <>
                if
                    "swap" op.execute
                then

                code.merge_stack_block

                i @ 1 - i !
                i @ 0<
            until
            "drop" op.execute
        then
    then
;



: , immediate description: "Separator in the [ index , ... ] and { key -> value , ... } syntaxes."
    "," sentinel_word
;



: ]! immediate description: "End of the [ index ] syntax.  Indicates an array write."
    "]!" sentinel_word
;



: ]!! immediate description: "End of the [ index ] syntax.  Indicates a an array variable write."
    "]!!" sentinel_word
;



: ]@ immediate description: "End of the [ index ] syntax.  Indicates an array read."
    "]@" sentinel_word
;



: ]@@ immediate description: "End of the [ index ] syntax.  Indicates an array variable read."
    "]@@" sentinel_word
;

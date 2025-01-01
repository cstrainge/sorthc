
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



: until immediate description: "The end of a loop/until block."
    "until" sentinel_word
;



: while immediate description: "Part of a begin/while/repeat block."
    "while" sentinel_word
;



: repeat immediate description: "The end of a begin/while/repeat block."
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



: loop immediate description: "The end of a do loop."
    "loop" sentinel_word
;



( Some useful stack management words. )



: drop description: "Drop the top item from the stack."
       signature: "a -- "
    variable! ignored
;



: swap description: "Swap the top two items on the stack."
       signature: "a b -- b a"
    variable! b
    variable! a

    b @
    a @
;



: over description: "Duplicate the second item on the stack."
       signature: "a b -- a b a"
    variable! b
    variable! a

    a @
    b @
    a @
;



: rot description: "Rotate the top three items on the stack."
      signature: "a b c -- b c a"
    variable! c
    variable! b
    variable! a

    a @
    b @
    c @
;



: dup description: "Duplicate the top item on the stack."
      signature: "a -- a a"
    variable! value

    value @
    value @
;



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



( A try/catch block for exception handling. )
: try immediate description: "Define the try/catch/endcatch syntax."
                signature: "try <code> catch <code> endcatch"
    unique_str variable! catch_label
    unique_str variable! end_catch_label

    code.new_block

    catch_label @ op.mark_catch
    "catch" 1 code.compile_until_words
    drop

    op.unmark_catch
    end_catch_label @ op.jump

    catch_label @ op.jump_target
    "endcatch" 1 code.compile_until_words
    drop

    end_catch_label @ op.jump_target

    code.resolve_jumps
    code.merge_stack_block
;



: catch immediate description: "End of the try block, starts the catch block."
    "catch" sentinel_word
;



: endcatch immediate description: "End of the total try/catch/endcatch block."
    "endcatch" sentinel_word
;



( Given an array and an operator go through the array and select out one of the values using that )
( operator. )
: one_of hidden  ( array operator -- chosen-value )
    variable! operator
    variable! values

    values [].size@@ constant size

    size  0<=
    if
        "No values in array." throw
    then

    values [ 0 ]@@ variable! chosen
    1 variable! index

    begin
        index @  size  <
    while
        values [ index @ ]@@ dup  chosen @  operator @ execute
        if
            chosen !
        else
            drop
        then

        index ++!
    repeat

    chosen @
;



: min_of description: "Get the minimum of an array of values."
         signature: "array -- smallest-value"
    ` < one_of
;



: max_of description: "Get the maximum of an array of values."
         signature: "array -- smallest-value"
    ` > one_of
;



: min description: "Get the minimum of two values."
      signature: "a b -- [a or b]"
    variable! b
    variable! a

    [ a @ , b @ ]  ` <  one_of
;



: max description: "Get the maximum of two values."
      signature: "a b -- [a or b]"
    variable! b
    variable! a

    [ a @ , b @ ]  ` >  one_of
;



: <> description: "Compare two values for inequality."
     signature: "a b -- are-not-equal?"
    = '
;



: ++  description: "Increment a value on the stack."
      signature: "value -- incremented"
    1 +
;



: ++!  description: "Increment the given variable."
       signature: "variable -- "
    dup @ ++ swap !
;



: --  description: "Decrement a value on the stack."
      signature: "value -- decremented"
    1 -
;



: --!  description: "Decrement the given variable."
       signature: "variable -- "
    dup @ -- swap !
;



: 0>  description: "Is the value greater than 0?"
      signature: "value -- test_result"
    0 >
;



: 0=  description: "Does the value equal 0?"
      signature: "value -- test_result"
    0 =
;



: 0<  description: "Is the value less than 0?"
      signature: "value -- test_result"
    0 <
;



: 0>=  description: "Is the value greater or equal to 0?"
       signature: "value -- test_result"
    0 >=
;



: 0<=  description: "Is the value less than or equal to 0?"
       signature: "value -- test_result"
    0 <=
;



: [&&] immediate  description: "Evaluate && at compile time."
                  signature: "a b -- result"
    &&
;



: [||] immediate  description: "Evaluate || at compile time."
                  signature: "a b -- result"
    ||
;



( If we have the user environment available, include some more useful words. )
: [is-windows?] immediate description: "Evaluate at compile time, is the OS Windows?"
                          signature: " -- bool"
    sorth.os  "Windows"  =
;



: [is-macos?] immediate description: "Evaluate at compile time, is the OS macOS?"
                        signature: " -- bool"
    sorth.os  "macOS"  =
;



: [is-linux?] immediate description: "Evaluate at compile time, is the OS Linux?"
                        signature: " -- bool"
    sorth.os  "Linux"  =
;


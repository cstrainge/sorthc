
( Collection of words for working with arrays. )



( The following words are implemented in the run-time library. )

( [].new )
( [].size@ )
( []! )
( []@ )
( [].insert )
( [].delete )
( [].size! )
( [].+ )
( [].= )
( [].push_front! )
( [].push_back! )
( [].pop_front! )
( [].pop_back! )



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

            code.stack-block-size@ 0 >
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

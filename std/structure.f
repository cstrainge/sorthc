
: # immediate description: "Beginning of a structure definition."
              signature: "# name field_name [ -> default_value ] ... ;"
    word variable! struct_name
    false variable! is_hidden
    variable field_name

    0 [].new variable! fields
    0 [].new variable! defaults
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

            index @ ++ fields [].size!!
            index @ ++ defaults [].size!!

            field_name @ fields [ index @ ]!!

            index ++!
        endcase
    repeat

    found_initializers @
    if
        true code.insert_at_front
        defaults @ op.push_constant_value
        false code.insert_at_front
    then

    struct_name @        op.push_constant_value
    fields @             op.push_constant_value
    is_hidden @          op.push_constant_value
    found_initializers @ op.push_constant_value

    "#" op.execute

    code.merge_stack_block
;



: #.new immediate description: "Create a new instance of the named structure."
                  signature: "#.new struct_name { field -> value , ... }"
    word variable! struct_name

    word dup "{" <>
    if
        "Expected { word to open structure creation, found " swap + "." + throw
    then
    drop

    struct_name @ ".new" + op.execute

    variable next_word
    variable field_name

    begin
        next_word @ "}" <>
    while
        word field_name !

        word dup "->" <>
        if
            "Expected assignment operator, ->, found " swap + "." + throw
        then
        drop

        "," "}" 2 code.compile_until_words
        next_word !

        "swap" op.execute
        "over" op.execute
        struct_name @ "." + field_name @ + op.execute
        "swap" op.execute
        "#!" op.execute
    repeat
;

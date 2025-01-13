
( Words for working with structures. )



( The following words are implemented in the run-time library. )

( #.is-of-type? )
( #@ )
( #! )
( #.iterate )
( #.field-exists? )
( #.= )



( The syntax handler for defining new structures... )
( Here we parse the incoming token stream and extract the structure name, field names and if )
( supplied the default value initialization code.  Once all this information is gathered, we pass )
( it off to the C++ code to actually define the new structure within the compiler. )
: # immediate description: "Beginning of a structure definition."
              signature: "# name field_name [ -> default_value ] ... ;"
    ( Get the name of the new structure, and assume that it will be visible in the user directory. )
    word variable! struct_name
    false variable! is_hidden

    ( Create a new array to hold the field names for the new structure. )
    0 [].new variable! fields

    ( Keep track of the current field name and index. )
    variable field_name
    0 variable! index

    ( Crate a new code block to hold any structure initialization code that the user may have )
    ( supplied.  We also keep track of if any code was actually found. )
    false variable! found_initializers

    code.new_block

    begin
        ( We keep going until a ; word is found or we run out of words in the token stream. )
        true
    while
        ( Grab the next word and determine what to do with it. Is it a field name or part of the )
        ( initialization code / structure syntax? )
        word field_name !

        field_name @
        case
            ( Mark this structure and it's words as hidden from the user directory. )
            "hidden" of
                    true is_hidden !
                    continue
                endof

            ( Make sure to exclude comments from the structure definition.  Because we are )
            ( grabbing tokens from the stream before the compiler can see them we need to )
            ( manually check for comments. )
            "(" of
                    ( Just call the current comment handler to deal with this. then move onto the )
                    ( next word in the token stream. )
                    "(" execute
                    continue
                endof

            ( We found a value initializer for the last field, so generate the code to properly )
            ( initialize the value when an instance of the structure is created. )
            "->" of
                    ( We definitely found some initialization code. )
                    true found_initializers !

                    ( The init code expects an array the same size of the structure to be on the )
                    ( stack with default values, (none) for each field. )

                    ( So the sequence is: )

                    ( dup the array... )
                    ( -- user init-code for the current value -- )
                    ( swap the array to the top of the stack. )
                    ( push the value index )
                    ( swap the array back to the top of the stack. )
                    ( []! to write the new value into the array at the field's position. )
                    ( Leaving a copy of the array on the stack for the next field or for returning )
                    ( to the calling code. )
                    "dup" op.execute

                    ( Compile the value initialization code. )
                    ";" "," 2 code.compile_until_words

                    "swap" op.execute
                    index @ -- op.push_constant_value
                    "swap" op.execute
                    "[]!" op.execute

                    ( If we hit the ; word while compiling user code that means that we're at the )
                    ( end of the structure definition, so break out of the loop. )
                    ";" =
                    if
                        break
                    then
                endof

            ( We've hit the end of the structure definition.  Break out of the loop. )
            ";" of
                    break
                endof

            ( The current token must have been a field name, so we grow the field name list and )
            ( record the new field name to it. )
            index @ ++ fields [].size!!
            field_name @ fields [ index @ ]!!

            ( Move onto the next field name index. )
            index ++!
        endcase
    repeat

    ( Did we find any initialization code? )
    found_initializers @
    if
        ( We found initialization code, so pack it up and pass it to the C++ structure creation )
        ( code.  Pop it off of the construction stack and onto the data stack. )
        code.pop_stack_block
    else
        ( No initialization code was found, so we don't need this block of code anymore. )
        ( Just drop it from the construction stack. )
        code.drop_stack_block
    then

    ( Pass the rest of the structure definition information on. )
    struct_name @
    fields @
    is_hidden @
    found_initializers @

    ( Finally call the C++ compiler run-time word to perform the actual registration. )
    #.register
;



( Generate code that can initialize a structure with new values. The syntax is very similar to )
( the structure definition syntax, with the -> word used to assign values to the fields. )
: #.new immediate description: "Create a new instance of the named structure."
                  signature: "#.new struct_name { field -> value , ... }"
    ( Grab teh name of the structure we're creating an instance of. )
    word variable! struct_name

    ( Make sure that the next word is the opening { word. )
    word dup "{" <>
    if
        "Expected { word to open structure creation, found " swap + "." + throw
    then
    drop

    ( Call the structure creation word. )
    struct_name @ ".new" + op.execute

    ( Go through the syntax until we hit the closing } word. )
    variable next_word
    variable field_name

    begin
        next_word @ "}" <>
    while
        ( Grab the next word we're assuming it is the field name. )
        word field_name !

        ( Make sure the next word is the assignment operator, ->. )
        word dup "->" <>
        if
            "Expected assignment operator, ->, found " swap + "." + throw
        then
        drop

        ( Compile the code to initialize the field with the new value. )
        "," "}" 2 code.compile_until_words
        next_word !

        ( Now that the new field value is on the top of the stack...  Swap it with the structure )
        ( instance so we can call the appropriate word to write the new value into the structure. )
        "swap" op.execute
        "over" op.execute
        struct_name @ "." + field_name @ + op.execute  ( Call the field index word. )
        "swap" op.execute                              ( Make sure that the structure value is at
                                                       ( top of the stack. )
        "#!" op.execute                                ( Write the new value into the structure. )
    repeat
;

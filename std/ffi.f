
( Define a new foreign function. )
: ffi.fn immediate
    word variable! original-name  ( The original name of the function. )
    "" variable! alias            ( The Forth alias to use for the function. )

    0 [].new variable! arguments  ( The argument types to the function. )

    ( Get the next word and determine what to do with it. )
    word variable! next

    ( Check to see if the 'as' keyword was supplied, indicating an alias. )
    word dup "as"  <>
    if
        ( We've been given an alias for the function. )
        word alias !

        ( Get the next word. )
        word next !
    then

    ( Gather the argument types for the function. )
    begin
        ( Keep going until we get the return value keyword, '->'. )
        next @ "->" <>
    while
        ( Add the argument type to the list. )
        next @  arguments [].push_back!!

        ( Get the next word. )
        word next !
    repeat

    ( Pass the function information to the compiler for registration. )
    original-name @
    alias @
    arguments @
    word              ( Get the return type. )

    ffi.register-function
;



( Define a new structure with type information for the foreign function interface. )
( The syntax is much the same as for regular structures, but with the addition of type information )
( for the member fields. )
: ffi.# immediate
    ( Get the name of the new structure. )
    word variable! struct-name

    ( The alignment of the structure. )
    variable alignment

    ( Create new arrays to hold the field names and their types for the new structure. )
    0 [].new variable! fields
    0 [].new variable! field-types

    ( Keep track of the last word extracted from the token stream, and the index of the current )
    ( field being extracted. )
    "" variable! next-word
    0 variable! field-index

    ( Create a new code block to hold any structure initialization code that the user may have )
    ( supplied.  We also keep track of if any code was actually found. )
    code.new_block
    false variable! found-initializers

    ( Go through the token stream until we find the end of the structure definition. )
    begin
        word next-word !  ( Get the next word. )

        ( Check to see if we've reached the end of the structure definition. )
        next-word @ ";" <>
    while
        ( Figure out what to do with the next word. )
        next-word @
        case
            "align" of
                    word alignment !
                    continue
                endof

            "(" of
                    ( Skip comments... )
                    "(" execute
                    continue
                endof

            "->" of
                    ( We definitely found some initialization code. )
                    true found-initializers !

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

            ( Resize the field name and type arrays to hold the new field. )
            index @ ++ fields [].size!!
            index @ ++ field-types [].size!!

            ( The next-word is the field type. )
            next-word @ field-types [ index @ ]!!

            ( Get the field name. )
            word field [ index @ ]!!
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

    ( Pass the structure information to the compiler for registration. )
    struct-name @
    alignment @
    fields @
    field-types @
    found-initializers @

    ffi.register-structure
;



: as immediate
    "as" sentinel_word
;



: ffi.void immediate
    "ffi.void" sentinel_word
;



: ffi.bool immediate
    "ffi.bool" sentinel_word
;



: ffi.i8 immediate
    "ffi.i8" sentinel_word
;



: ffi.u8 immediate
    "ffi.u8" sentinel_word
;



: ffi.i16 immediate
    "ffi.i16" sentinel_word
;



: ffi.u16 immediate
    "ffi.u16" sentinel_word
;



: ffi.i32 immediate
    "ffi.i32" sentinel_word
;



: ffi.u32 immediate
    "ffi.u32" sentinel_word
;



: ffi.f32 immediate
    "ffi.f32" sentinel_word
;



: ffi.f64 immediate
    "ffi.f64" sentinel_word
;



: ffi.string immediate
    "ffi.string" sentinel_word
;



: ffi.void-ptr immediate
    "ffi.void-ptr" sentinel_word
;
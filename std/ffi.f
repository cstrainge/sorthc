
( Define a new foreign function. )
: ffi.fn immediate
    word variable! original-name  ( The original name of the function. )
    "" variable! alias            ( The Forth alias to use for the function. )

    0 [].new variable! arguments  ( The argument types to the function. )
    -1 variable! var-arg          ( Flag for if the function is variadic, and what param is the )
                                  ( starting one. )

    ( Get the next word and determine what to do with it. )
    word variable! next

    ( Check to see if the 'as' keyword was supplied, indicating an alias. )
    next @ "as" =
    if
        ( We've been given an alias for the function. )
        word alias !

        ( Get the next word. )
        word next !
    then

    0 variable! arg-index

    ( Gather the argument types for the function. )
    begin
        ( Keep going until we get the return value keyword, '->'. )
        next @ "->" <>
    while
        ( Is this the var-arg flag? )
        next @ "ffi.var-arg" =
        if
            ( It is, so set the flag and the starting index. )
            arg-index @ var-arg !
        else
            ( Add the argument type to the list. )
            next @ arguments [].push_back!!
        then

        ( Get the next word. )
        word next !
        arg-index ++!
    repeat

    ( Pass the function information to the compiler for registration. )
    original-name @
    alias @
    var-arg @
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
    "8" variable! alignment

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
                    field-index @ -- op.push_constant_value
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
            field-index @ ++ fields [].size!!
            field-index @ ++ field-types [].size!!

            ( The next-word is the field type. )
            next-word @ field-types [ field-index @ ]!!

            ( Get the field name. )
            word fields [ field-index @ ]!!

            field-index ++!
        endcase
    repeat

    ( Did we find any initialization code? )
    found-initializers @
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



( Define an external variable for use in Forth code.  We'll create reader and writer words for the )
( variable.  Which the user uses to access the variable, because there needs to be something that )
( performs the conversion from the internal value representation to the native representation. )
( )
( The syntax of the definition is: )
( )
( ffi.var name as reader-word , writer-word -> var-type )
: ffi.var immediate
    ( Get the name of the variable. )
    word variable! var-name

    ( Make sure that we got the as keyword. )
    word variable! next
    next @ "as" <>
    if
        next @ "Expected 'as' but found {}." string.format throw
    then

    ( Get the reader word name. )
    word variable! reader-word

    ( Make sure that we got the , keyword. )
    word next !
    next @ "," <>
    if
        next @ "Expected ',' but found {}." string.format throw
    then

    ( Get the writer word name. )
    word variable! writer-word

    ( Make sure that we got the -> keyword. )
    word next !
    next @ "->" <>
    if
        next @ "Expected '->' but found {}." string.format throw
    then

    word variable! type-name

    ( Pass the variable information to the compiler for registration. )
    type-name @
    writer-word @
    reader-word @
    var-name @

    ffi.register-var
;



( Define an array type for the FFI. )
: ffi.[] immediate
    word variable! array-name
    -1 variable! array-length

    word variable! next

    ( Check to see if we were given a size or not. )
    next @ "->" <>
    if
        ( Looks like we were given a fixed length for the array type. )
        next @ string.to-number array-length !

        ( The next word now needs to be the -> keyword. )
        word dup "->" <>
        if
            "Expected '->' but found " swap + "." + throw
        then
        drop

        ( Get the next word for the type name. )
        word next !
    then

    ( Read the type name. )
    next @ variable! value-type-name

    ( Check to see if we're treating the array as a string or not. )
    word next !
    false variable! treat-as-string

    ( Check to see if we're treating the array as a string or not. )
    next @ "as" =
    if
        ( We only support strings for now. )
        word "ffi.string" <>
        if
            "Expected 'ffi.string' but found " next @ + "." + throw
        then

        ( Make sure that the underlying type of the array makes sense. )
        value-type-name @ "ffi.u8" <>
        if

            "Expected the array " array-name @ +
            " to be 'ffi.u8' but is " +
            value-type-name @ +
            "." +
            throw
        then

        true treat-as-string !
        word next !
    then

    ( Make sure that the definition ends with a semicolon. )
    next @ ";" <>
    if
        "Expected a ';' but found " next @ + "." + throw
    then

    ( Pass the array type information to the compiler for registration. )

    array-name @
    array-length @
    value-type-name @
    treat-as-string @

    ffi.register-array-type
;



: as immediate
    "as" sentinel_word
;



: align immediate
    "align" sentinel_word
;



: ffi.var-arg immediate
    "ffi.var-arg" sentinel_word
;



: ffi.void immediate
    "ffi.void" sentinel_word
;

: ffi.void:ptr immediate
    "ffi.void:ptr" sentinel_word
;

( ffi.void:out.ptr is unsupported. )

( ffi.void:in/out.ptr is unsupported. )



: ffi.bool immediate
    "ffi.bool" sentinel_word
;

: ffi.bool:ptr immediate
    "ffi.bool:ptr" sentinel_word
;

: ffi.bool:out.ptr immediate
    "ffi.bool:out.ptr" sentinel_word
;

: ffi.bool:in/out.ptr immediate
    "ffi.bool:in/out.ptr" sentinel_word
;



: ffi.i8 immediate
    "ffi.i8" sentinel_word
;

: ffi.i8:ptr immediate
    "ffi.i8:ptr" sentinel_word
;

: ffi.i8:out.ptr immediate
    "ffi.i8:out.ptr" sentinel_word
;

: ffi.i8:in/out.ptr immediate
    "ffi.i8:in/out.ptr" sentinel_word
;



: ffi.u8 immediate
    "ffi.u8" sentinel_word
;

: ffi.u8:ptr immediate
    "ffi.u8:ptr" sentinel_word
;

: ffi.u8:out.ptr immediate
    "ffi.u8:out.ptr" sentinel_word
;

: ffi.u8:in/out.ptr immediate
    "ffi.u8:in/out.ptr" sentinel_word
;



: ffi.i16 immediate
    "ffi.i16" sentinel_word
;

: ffi.i16:ptr immediate
    "ffi.i16:ptr" sentinel_word
;

: ffi.i16:out.ptr immediate
    "ffi.i16:out.ptr" sentinel_word
;

: ffi.i16:in/out.ptr immediate
    "ffi.i16:in/out.ptr" sentinel_word
;



: ffi.u16 immediate
    "ffi.u16" sentinel_word
;

: ffi.u16:ptr immediate
    "ffi.u16:ptr" sentinel_word
;

: ffi.u16:out.ptr immediate
    "ffi.u16:out.ptr" sentinel_word
;

: ffi.u16:in/out.ptr immediate
    "ffi.u16:in/out.ptr" sentinel_word
;



: ffi.i32 immediate
    "ffi.i32" sentinel_word
;

: ffi.i32:ptr immediate
    "ffi.i32:ptr" sentinel_word
;

: ffi.i32:out.ptr immediate
    "ffi.i32:out.ptr" sentinel_word
;

: ffi.i32:in/out.ptr immediate
    "ffi.i32:in/out.ptr" sentinel_word
;



: ffi.u32 immediate
    "ffi.u32" sentinel_word
;

: ffi.u32:ptr immediate
    "ffi.u32:ptr" sentinel_word
;

: ffi.u32:out.ptr immediate
    "ffi.u32:out.ptr" sentinel_word
;

: ffi.u32:in/out.ptr immediate
    "ffi.u32:in/out.ptr" sentinel_word
;



: ffi.f32 immediate
    "ffi.f32" sentinel_word
;

: ffi.f32:ptr immediate
    "ffi.f32:ptr" sentinel_word
;

: ffi.f32:out.ptr immediate
    "ffi.f32:out.ptr" sentinel_word
;

: ffi.f32:in/out.ptr immediate
    "ffi.f32:in/out.ptr" sentinel_word
;



: ffi.f64 immediate
    "ffi.f64" sentinel_word
;

: ffi.f64:ptr immediate
    "ffi.f64:ptr" sentinel_word
;

: ffi.f64:out.ptr immediate
    "ffi.f64:out.ptr" sentinel_word
;

: ffi.f64:in/out.ptr immediate
    "ffi.f64:in/out.ptr" sentinel_word
;



: ffi.string immediate
    "ffi.string" sentinel_word
;

: ffi.string:ptr immediate
    "ffi.string:ptr" sentinel_word
;

: ffi.string:out.ptr immediate
    "ffi.string:out.ptr" sentinel_word
;

: ffi.string:in/out.ptr immediate
    "ffi.string:in/out.ptr" sentinel_word
;

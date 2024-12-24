
: {}!! description: "Insert a value into the hash table variable."
       signature: "value key hash_variable -- "
    @ {}!
;



: {}@@ description: "Read a value from the hash table variable."
       signature: "key hash_variable -- value"
    @ {}@
;



: {}?? description: "Does a given key exist within the hash table variable?"
       signature: "key hash_variable -- does_exist?"
    @ {}?
;



: { immediate description: "Define both the 'hash { key }'  and '{ key -> value , ... }' syntaxes."
              signature: "hash { key }<operation> *or* { key -> value , ... }"
    variable command

    false variable! is_new
    false variable! is_inline_syntax

    "->" "}" "}!" "}!!" "}@" "}@@" 6 code.compile_until_words

    case
        "->"  of  true is_inline_syntax !                 endof
        "}"   of  ( "Missing word -> and key value." throw )  endof
        "}!"  of  ` {}!  command !                        endof
        "}!!" of  ` {}!! command !                        endof
        "}@"  of  ` {}@  command !                        endof
        "}@@" of  ` {}@@ command !                        endof
    endcase

    false is_inline_syntax @ =
    if
        ` swap op.execute
        command @ op.execute
    else
        ` {}.new op.execute
        ` over op.execute
        "," "}" 2 code.compile_until_words
        ` rot op.execute
        ` {}! op.execute

        ` dup op.execute

        "}" <>
        if
            begin
                "->" "}" 2 code.compile_until_words
                "->" =
            while
                ` swap op.execute
                "," "}" 2 code.compile_until_words
                ` rot op.execute
                ` {}! op.execute
                ` dup op.execute

                "}" =
                if
                    break
                then
            repeat
        then

        ` drop op.execute
    then
;



: }! immediate description: "End of the { key } syntax.  Indicates a hash table write."
    "}!" sentinel_word
;



: }!! immediate description: "End of the { key } syntax.  Indicates a a hash table variable write."
    "}!!" sentinel_word
;



: }@ immediate description: "End of the { key } syntax.  Indicates a hash table read."
    "}@" sentinel_word
;



: }@@ immediate description: "End of the { key } syntax.  Indicates a hash table variable read."
    "}@@" sentinel_word
;



: } immediate description: "Hash table definition syntax."
    "}" sentinel_word
;

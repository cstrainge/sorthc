
( Extra words for value type info and comparison. )



( The following words are implemented in the run-time library. )

( value.is-number? )
( value.is-boolean? )
( value.is-string? )
( value.is-structure? )
( value.is-array? )
( value.is-buffer? )
( value.is-hash-table? )
( value.copy )
( value.to-string )
( hex )



: value.both-are? description: "Check if the two values are the same type."
                  signature: "a b value-check -- are-same-type?"
    variable! operation
    variable! b
    variable! a

    a @  operation @  execute
    b @  operation @  execute
    &&
;



: value.both-are-hash-tables? description: "Are two values hash tables?"
                              signature: "a b -- are-hash-tables?"
    ` value.is-hash-table?  value.both-are?
;



: value.both-are-arrays? description: "Are two values arrays?"
                         signature: "a b -- are-arrays?"
    ` value.is-array?  value.both-are?
;



: value.both-are-structures? description: "Are two values structures?"
                             signature: "a b -- are-structures?"
    ` value.is-structure?  value.both-are?
;



: value.both-are-strings? description: "Are two values strings?"
                          signature: "a b -- are-strings?"
    ` value.is-string?  value.both-are?
;



: value.both-are-numbers? description: "Are two values numbers?"
                          signature: "a b -- are-numbers?"
    ` value.is-number?  value.both-are?
;



: value.both-are-booleans? description: "Are two values booleans?"
                           signature: "a b -- are-boolean?"
    ` value.is-boolean?  value.both-are?
;



: = description: "Compare two values for equality"
    signature: "a b -- are_equal?"
    variable! b
    variable! a

    a @ b @  value.both-are-structures?
    if
        a @ b @  #.=
    else
        a @ b @  value.both-are-hash-tables?
        if
            a @ b @  {}.=
        else
            a @ b @  value.both-are-arrays?
            if
                a @ b @  [].=
            else
                a @ b @  =
            then
        then
    then
;



: <> description: "Compare two values for inequality."
     signature: "a b -- are-not-equal?"
    = '
;



( Extend the + operator to include hash tables and arrays. )
: + description: "Add two values together."
    signature: "a b -- result"
    variable! b
    variable! a

    a @ b @  value.both-are-hash-tables?
    if
        a @ b @  {}.+
    else
        a @ b @  value.both-are-arrays?
        if
            a @ b @  [].+
        else
            a @ b @  +
        then
    then
;

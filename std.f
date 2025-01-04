
[include] std/core-words.f



( Now that we've included the core words, we have the ability to comment the code now. )
( This is the standard library for the Sorth programming language.  For words that are available )
( the compiler's run-time environment for execution by immediate words, see std/core-words.f.)



( Value comparison words. )
[include] std/values.f



( String manipulation words. )
[include] std/strings.f



( Structure words. )
[include] std/structure.f



( Array words. )
[include] std/array.f



( Hash table words. )
[include] std/hash-table.f



( Include some words for accessing the terminal. )
[include] std/terminal.f



( Simple words for printing to the terminal. )
[include] std/printing.f



( Include some useful words for accessing the user's environment. )
[include] std/user.f



( Include words for JSON parsing and generation. )
[include] std/json.f



( The foreign function interface. )
[include] std/ffi.f



( Hack for compiling scripts. )



: #!/usr/bin/env
    ( Do nothing. )
;



: sorth
    ( Do nothing. )
;


: string.split description: "Given a split character, split a string into an array of strings."
               signature: "split_char string -- string_array"
    variable! string
    constant splitter

    string @ string.size@ constant string_size

    [ "" ] variable! output
    0 variable! output_index

    0 variable! index
    variable next

    begin
        index @  string_size  <
    while
        index @ string string.[]@@ next !

        splitter  next @  =
        if
            output [].size++!!
            output_index ++!
            "" output [ output_index @ ]!!
        else
            output [ output_index @ ]@@ next @ +  output [ output_index @ ]!!
        then

        index ++!
    repeat

    output [ output_index @ ]@@  string.size@  0=
    if
        output [].size--!!
    then

    output @
;



( Given a format string and a position after the beginning bracket { extract the substring found )
( within those brackets, {}  If there no specifier there an empty string, "", is returned instead. )
( in both cases an updated index is also returned that points to after the closing bracket, }. )
: string.format.extract_specifier  hidden  ( index format_string -- new_index specifier_string )
    variable! format_str
    variable! char_index
    format_str @ string.size@ variable! length

    "" variable! specifier
    variable next

    char_index @   format_str @  string.[]@  "}" <>
    if
        begin
            char_index @  length @  <
        while
            char_index @  format_str @  string.[]@  next !

            next @  "}"  <>
            if
                specifier @  next @  +  specifier !
            else
                break
            then

            char_index ++!
        repeat

        next @  "}"  <>
        if
            "Missing closing } in format specifier." throw
        then
    then

    char_index @
    specifier @
;



( Get a character from the string at the given index.  If the index is outside of the string an )
( empty string is returned instead. )
: string.format.get_char  hidden  ( index string -- character )
    variable! string
    variable! index

    string @ string.size@ variable! size

    "" variable! char

    index @  size @  <
    if
        index @  string string.[]@@  char !
    then

    char @
;



( Parse the specifier found in the format string and return it's values broken out.  If the string )
( is empty or a component is missing then the default is returned in it's stead. )
: string.format.parse_specifier  hidden  ( value specifier -- fill alignment width is_hex )
    variable! specifier
    variable! value

    " " variable! fill
    value @ value.is-number? if ">" else "<" then variable! alignment
    ""  variable! width
    false variable! is_hex

    variable char
    0 variable! index

    specifier string.size@@ variable! size

    specifier @  ""  <>
    if
        index @ specifier @ string.format.get_char char !

        char @  "<"  =  char @  "^"  =  ||  char @  ">"  =  ||
        if
            char @  alignment !
            index ++!
        then

        index @ specifier @ string.format.get_char char !

        char @  "0"  <  char @  "9"  >  ||
        char @  "0"  =
        ||
        if
            char @ fill !
            index ++!
        then

        index @ specifier @ string.format.get_char char !

        begin
            char @  "0"  >=  char @  "9"  <=  &&
            index @  size @  <
            &&
        while
            width @ char @ +  width !

            index ++!
            index @ specifier @ string.format.get_char char !
        repeat

        width @ string.size@ 0>
        if
            width @ string.to_number width !
        else
            0 width !
        then

        index @ specifier @ string.format.get_char char !

        char @  "x"  =
        char @  "X"  =
        ||
        is_hex !
    else
        0 width !
    then

    fill @
    alignment @
    width @
    is_hex @
;



( Given a count and a character create a string of count width filled with that character. )
: string.format.fill_str  hidden  ( count char -- fill_string )
    variable! char
    variable! count

    0 variable! index

    "" variable! new_str

    begin
        index @  count @  <
    while
        new_str @  char @  +  new_str !
        index ++!
    repeat

    new_str @
;



( Given a value and a sub-specifier convert the value to a string and format it according to the )
( specifier string. )
: string.format_value  hidden  ( value specifier -- formatted_value )
    variable! specifier
    variable! value

    variable fill
    variable alignment
    variable width
    variable is_hex

    0 variable! fill_width

    variable str_value

    specifier @  ""  <>
    if
        value @ specifier @ string.format.parse_specifier is_hex ! width ! alignment ! fill !

        is_hex @
        if
            value @ value.is-number?
            if
                value @ hex str_value !
            else
                "Can't convert value to a hex string." throw
            then
        else
            value @ value.to-string str_value !
        then

        str_value string.size@@  width  <
        if
            width @ str_value string.size@@ -  fill_width !

            alignment @
            case
                "<" of
                        str_value @  fill_width @ fill @ string.format.fill_str  +  str_value !
                    endof

                "^" of
                        fill_width @ 2 / fill @ string.format.fill_str  str_value @  +  str_value !

                        fill_width @ 2 %  0  <>
                        if
                            fill_width @ 2 / 1 + fill_width !
                        else
                            fill_width @ 2 / fill_width !
                        then

                        str_value @  fill_width @ fill @ string.format.fill_str  +  str_value !
                    endof

                ">" of
                        fill_width @ fill @ string.format.fill_str  str_value @  +  str_value !
                    endof
            endcase
        then
    else
        value @ value.to-string str_value !
    then

    str_value @
;



: string.format
    description: "Format a string where occurrences of {} are replaced with stack values."
    signature: "[variables] format_string -- formatted_string"

    0 [].new variable! snippets
    0 [].new variable! values
    0 [].new variable! specifiers

    variable! format_str

    format_str string.size@@ variable! length
    0 variable! char_index
    "" variable! format_snippet

    variable next

    begin
        char_index @  length @  <
    while
        char_index @  format_str @  string.[]@  next !

        next @  "{"  =
        if
            char_index @ ++ format_str @ string.format.extract_specifier
            specifiers [].push_back!!
            char_index !

            format_snippet @  snippets  [].push_back!!
                                values  [].push_front!!

            "" format_snippet !
        else
            format_snippet @  next @  +  format_snippet !
        then

        char_index ++!
    repeat

    0 variable! snippet_index
    "" variable! output_string

    begin
        snippet_index @  snippets [].size@@  <
    while
        output_string @  snippets [ snippet_index @ ]@@  +  output_string !


        snippet_index @  values [].size@@  <
        if
            output_string @
            values [ snippet_index @ ]@@  specifiers [ snippet_index @ ]@@  string.format_value
            +

            output_string !
        then

        snippet_index ++!
    repeat

    format_snippet string.size@@  0>
    if
        output_string @ format_snippet @  +  output_string !
    then

    output_string @
;

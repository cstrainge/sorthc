
( Words for printing values to the terminal. )



: .  description: "Print a value to the terminal."
     signature: "value -- "
    term.!
;



: .hex  description: "Print a numeric value as hex."
        signature: "value -- "
    hex .
;



: cr  description: "Print a newline to the console."
      signature: " -- "
    "\n" term.!
    term.flush
;



: .cr  description: "Print a value and a new line."
       signature: "value -- "
    . cr
;



: .hcr  description: "Print a hex value and a new line."
        signature: "value -- "
    .hex cr
;



: .sp  description: "Print the given number of spaces."
       signature: "count -- "
    begin
        -- dup 0 >=
    while
        " " term.!
    repeat
    drop
;

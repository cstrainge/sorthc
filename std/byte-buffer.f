
( Helper words for reading/writing byte buffers. )



( The following words are implemented in the run-time library. )

( buffer.new )
( buffer.int! )
( buffer.int@ )
( buffer.float! )
( buffer.float@ )
( buffer.string! )
( buffer.string@ )
( buffer.size@ )
( buffer.position! )
( buffer.position@ )



: buffer.i8!! description: "Write an 8-bit signed integer to the buffer variable."
              signature: "value buffer_variable -- "
    @ 1 buffer.int!
;



: buffer.i16!! description: "Write a 16-bit signed integer to the buffer variable."
               signature: "value buffer_variable -- "
    @ 2 buffer.int!
;



: buffer.i32!! description: "Write a 32-bit signed integer to the buffer variable."
               signature: "value buffer_variable -- "
    @ 4 buffer.int!
;



: buffer.i64!! description: "Write a 64-bit signed integer to the buffer variable."
               signature: "value buffer_variable -- "
    @ 8 buffer.int!
;



: buffer.i8@@ description: "Read an 8-bit signed integer from the buffer variable."
              signature: "buffer_variable -- value"
    @ 1 true buffer.int@
;



: buffer.i16@@ description: "Read a 16-bit signed integer from the buffer variable."
               signature: "buffer_variable -- value"
    @ 2 true buffer.int@
;



: buffer.i32@@ description: "Read a 32-bit signed integer from the buffer variable."
               signature: "buffer_variable -- value"
    @ 4 true buffer.int@
;



: buffer.i64@@ description: "Read a 64-bit signed integer from the buffer variable."
               signature: "buffer_variable -- value"
    @ 8 true buffer.int@
;



: buffer.u8@@ description: "Read an 8-bit unsigned integer from the buffer variable."
              signature: "buffer_variable -- value"
    @ 1 false buffer.int@
;



: buffer.u16@@ description: "Read a 16-bit unsigned integer from the buffer variable."
               signature: "buffer_variable -- value"
    @ 2 false buffer.int@
;



: buffer.u32@@ description: "Read a 32-bit unsigned integer from the buffer variable."
               signature: "buffer_variable -- value"
    @ 4 false buffer.int@
;



: buffer.u64@@ description: "Read a 64-bit unsigned integer from the buffer variable."
               signature: "buffer_variable -- value"
    @ 8 false buffer.int@
;



: buffer.f32!! description: "Write a 32-bit floating point value to the buffer variable."
               signature: "buffer_variable -- value"
    @ 4 buffer.float!
;



: buffer.f64!! description: "Write a 64-bit floating point value to the buffer variable."
               signature: "buffer_variable -- value"
    @ 8 buffer.float!
;



: buffer.f32@@ description: "Read a 32-bit floating point value from the buffer variable."
               signature: "buffer_variable -- value"
    @ 4 buffer.float@
;



: buffer.f64@@ description: "Read a 64-bit floating point value from the buffer variable."
               signature: "buffer_variable -- value"
    @ 8 buffer.float@
;



: buffer.string!!
    description: "Write a string of a given size to the buffer variable.  Pad with 0s."
    signature: "string buffer_variable max_size -- "
    @ swap buffer.string!
;



: buffer.string@@ description: "Read a string of max size from the buffer variable."
                  signature: "buffer_variable max_size -- string"
    @ swap buffer.string@
;



: buffer.position!! description: "Set the current buffer pointer to the buffer in variable."
                    signature: "new_position buffer_variable -- "
    @ buffer.position!
;



: buffer.position@@ description: "Read the current buffer pointer from the variable."
                    signature: "buffer_variable -- position"
    @ buffer.position@
;

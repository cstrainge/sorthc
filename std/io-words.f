
( Collection of words for reading and writing files and sockets. )



( Error codes. )
4   constant posix.EINTR



( File permission flags. )
64  constant posix.O_CREAT
512 constant posix.O_TRUNC

256 constant posix.S_IRUSR
128 constant posix.S_IWUSR
32  constant posix.S_IRGRP
4   constant posix.S_IROTH

1   constant posix.F_GETFD

0   constant posix.SEEK_SET
1   constant posix.SEEK_CUR
2   constant posix.SEEK_END



( As of now, wrappers for a few of the posix functions are implemented in the runtime library. )
( Hopefully they'll be moved out at some point in the future as the FFI matures. )



( Import some POSIX functions. )

( posix.open is implemented in the run-time library. )

( posix.errno is implemented in the run-time library. )

( posix.set-errno is implemented in the run-time library. )

( posix.fcntl is implemented in the run-time library. )

( posix.read-buffer is implemented in the run-time library. )

( posix.write-buffer is implemented in the run-time library. )

ffi.fn fchmod as posix.chmod ffi.i32 ffi.u32 -> ffi.i32

ffi.fn close as posix.close ffi.i32 -> ffi.i32

ffi.fn lseek as posix.lseek ffi.i32 ffi.i32 ffi.i32 -> ffi.i32



( File open modes. )
0 constant file.r/o   ( Open or create a file as read-only. )
1 constant file.w/o   ( Open or create a file as write-only. )
2 constant file.r/w   ( Open or create a file as read-write. )



( The internal handler for calling the word posix.open. )
: file.call-posix-open  ( path mode extra-flags -- file-fd )
    variable! extra-flags ( Should be either posix.O_CREAT or posix.O_TRUNC. )
    variable! mode        ( Reading writing, or both? )
    variable! path        ( The path to the file. )

    ( The file descriptor for the open file. )
    -1 variable! file-fd

    ( Check to see if the file already exists. )
    path @ file.exists? variable! is-new?

    ( Attempt to open the file, keeping in mind we may be interrupted by a signal. )
    begin
        file-fd -1 =
        posix.errno posix.EINTR =
        &&
    while
        path@ mode extra-flags posix.open file-fd !
    repeat

    ( Were we able to open the file? )
    file-fd @ -1 =
    if
        path @ "Unable to open file {}." string.format throw
    then

    ( If the file didn't exist before, set some reasonable file permissions. )
    is-new? @
    if
        file-fd @ posix.S_IRUSR posix.S_IWUSR | posix.S_IRGRP | posix.S_IROTH | posix.chmod

        0 <>
        if
            file-fd @ file.close

            path @ "Unable to set file permissions on {}." string.format throw
        then
    then
;



( Open an existing file for access.  Pass one of file.r/o, file.w/o, or file.r/w as the mode. )
: file.open  ( path mode -- file-id )
    posix.O_CREAT file.call-posix-open
;



( Create a new file for access.  Pass one of file.r/o, file.w/o, or file.r/w as the mode. )
: file.create  ( path mode -- file-id )
    posix.O_TRUNC file.call-posix-open
;


( Close an open file. )
: file.close ( file-id -- )
    variable! file-fd
    -1 variable! result

    ( Clear the last error, if any. )
    0 posix.set-errno

    ( Attempt to close the file, keeping in mind we may be interrupted by a signal. )
    begin
        result @ -1 =
        posix.errno posix.EINTR =
        &&
    while
        file-fd @ posix.close result !
    repeat

    ( Were we able to close the file? )
    result @ -1 =
    if
        file-fd @ posix.errno "Unable to close file {}, error code {}." string.format throw
    then
;



( socket.connect is implemented in the run-time library. )



( file.size@ is implemented in the run-time library. )



( file.exists? is implemented in the run-time library. )



( Check to see if a given file descriptor is considered open? )
: file.is-open?  ( file-id -- is-open? )
    ( TODO: Check for path or fd. )
    posix.F_GETFD posix.fcntl -1 <>
;



( Get the current cursor position in the file. )
: file.position@  ( file-id -- position )
    ( Seek to the current position, thus returning the current position. )
    0 posix.SEEK_CUR posix.lseek
;



( Check to see if the given file descriptor is at the end of the file. )
: file.is-eof?  ( file-id -- is-eof? )
    variable! file-fd
    file-fd @ file.size@ variable! size

    file_fd @ file.position@ size @ >=
;



( Read a block of data from the file.  The read will attempt to fill the buffer.  But will fall )
( short if the end of the file is reached. )
: file.@  ( byte-count file-id -- buffer )
    variable! file-fd
    variable! byte-count

    ( Create a new buffer to hold the read data and fill it from the file, returning the new )
    ( buffer to the caller. )
    byte-count @ buffer.new dup file-fd @ posix.read-buffer
;



( Read a string of a given length from the file.  The read will fail if the file doesn't have )
( enough bytes to accommodate the read. )
: file.string@  ( char-count file-id -- string )
    variable! file-fd
    variable! char-count

    ( Read the requested number of characters from the file, returning a byte-buffer. )
    char-count @ file-fd @ file.@ variable! buffer

    ( Check to see if we've reached the end of the file. )
    buffer @ buffer.position@ char-count @ <>
    if
        "End of file reached." throw
    then

    ( Move the cursor back to the beginning of the buffer, and extract the new string. )
    0 buffer @ buffer.position!
    char-count @ buffer @ buffer.string@
;



( Read a single character from the file.  The read will fail if we're at the end of the file. )
: file.char@  ( file-id -- char )
    ( Swap the parameters to call with, 1 file-id file.string@ )
    1 swap file.string@
;



( Attempt to read a line of text from the file, terminated by a \n character.  The read will also )
( terminate if the end of the file is reached. So, it's entirely possible to return an empty )
( string. )
: file.line@ ( file-id -- string )
    variable! file-fd  ( Get the file descriptor from the caller. )

    ( Create new buffers to hold the read data. )
    "" variable! line
    "" variable! next-char

    begin
        ( Keep going until we've hit a \n character or the end of the file. )
        next-char @ "\n" <>
        file-fd @ file.is-eof? '
        &&
    while
        ( Append the current character to the gathered line. )
        next-char @ line @ +  line !

        ( Read the next character for the next iteration. )
        file-fd @ file.char@  next-char !
    repeat

    ( Return the gathered line. )
    line @
;



( Write a value as text, or a byte-buffer as binary. )
: file.!  ( value file-id -- )
    variable! file-id
    variable! value

    ( If the value isn't a buffer, convert it to a string and write it to one. )
    value @ buffer? '
    if
        ( Make sure that value is a string, and create a new buffer to hold it. )
        value @ value.to-string
        dup string.size@ dup buffer.new value !

        ( Write the string to the buffer. )
        ( Note, buffer.string! expects it's parameters to be passed as string buffer size. )
        ( string size ) value @ swap buffer.string!
    then

    ( Now that value is a byte-buffer, write it to the file. )
    buffer @ file-fd @ posix.write-buffer -1 =
    If
        file-fd @ posix.errno "Unable to write to fd {}, error code {}." string.format throw
    then
;



( Write a string to the file appending a new \n character at the end of the write. )
: file.line!  ( value file-id -- )
    variable! value

    ( Is the value already a buffer? )
    value @ value.is-buffer?
    if
        ( Just write it, and then write a \n separately. )
        value @ file.!
        "\n" file.!
    else
        ( Make sure that the value is a string with the \n character appended.  Then write it to )
        ( the file. )
        value @ value.to-string "\n" + file.!
    then
;

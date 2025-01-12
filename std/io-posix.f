
( Collection of higher level words for reading and writing files and sockets. )



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
    -1 variable! chmod-error

    ( Set some default values for the file permissions if we end up creating the file. )
    posix.S_IRUSR posix.S_IWUSR | posix.S_IRGRP | posix.S_IROTH | constant file-permissions

    ( Clear the last error, if any. )
    0 posix.errno!

    ( Check to see if the file already exists. )
    path @ file.exists? variable! is-existing?

    ( Attempt to open the file, keeping in mind we may be interrupted by a signal. )
    begin
        path @  mode @ extra-flags @ |  posix.open  file-fd !

        file-fd @ -1 <>
        posix.errno@ posix.EINTR <>
        ||
    until

    ( Were we able to open the file? )
    file-fd @ -1 =
    if
        path @ posix.errno@ posix.strerror "Unable to open file {}: {}." string.format throw
    then

    ( If the file didn't exist before, set some reasonable file permissions. )
    is-existing? @ '
    if
        begin
            file-fd @  file-permissions  posix.fchmod  chmod-error !

            chmod-error @ -1 <>
            posix.errno@ posix.EINTR <>
            ||
        until

        chmod-error @ -1 =
        if
            file-fd @ file.close

            path @
            posix.errno@ posix.strerror
            "Unable to set file permissions on {}: {}." string.format throw
        then
    then

    file-fd @
;



( Open an existing file for access.  Pass one of file.r/o, file.w/o, or file.r/w as the mode. )
: file.open  ( path mode -- file-id )
    posix.O_TRUNC file.call-posix-open
;



( Create a new file for access.  Pass one of file.r/o, file.w/o, or file.r/w as the mode. )
: file.create  ( path mode -- file-id )
    posix.O_CREAT file.call-posix-open
;



( Connect to a server's existing socket. )
: socket.connect  ( path -- fd )
    variable! path

    posix.AF_UNIX posix.SOCK_STREAM posix.socket variable! fd

    fd @ -1 =
    if
        posix.errno@ posix.strerror "Unable to create socket: {}." string.format throw
    then

    posix.sockaddr_un.new variable! sock-addr

    posix.AF_UNIX sock-addr posix.sockaddr_un.sun_family!!
    path @ sock-addr posix.sockaddr_un.sun_path!!

    fd @ sock-addr @ posix.sockaddr_un.size@ posix.connect -1 =
    if
        posix.errno@ posix.strerror "Unable to connect to socket: {}." string.format throw
    then

    fd @
;



( Close an open file. )
: file.close  ( file-id -- )
    variable! file-fd
    -1 variable! result

    ( We'll be checking for posix.EINTR... )
    posix.EINTR posix.errno!

    ( Attempt to close the file, keeping in mind we may be interrupted by a signal. )
    begin
        result @ -1 =
        posix.errno@ posix.EINTR =
        &&
    while
        file-fd @ posix.close result !
    repeat

    ( Were we able to close the file? )
    result @ -1 =
    if
        file-fd @ posix.errno@ posix.strerror "Unable to close file {}, {}." string.format throw
    then
;



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
    variable! total-size

    ( Create a new buffer to hold the read data. )
    total-size @ buffer.new variable! buffer
    0 variable! read-bytes

    ( Attempt to read the requested number of bytes from the file, keeping in mind we may be )
    ( interrupted by a signal. )
    begin
        0 posix.errno!

        ( Read the requested number of bytes from the file, and keep track of how many bytes were )
        ( actually read. )
        file-fd @  buffer @  total-size @ read-bytes @ -  posix.read  read-bytes !

        ( Check to see if the read was successful. )
        read-bytes @ 0 >=
        if
            ( Update the buffer's position to reflect the number of bytes read. )
            buffer @  buffer @ buffer.position@ read-bytes @ +  buffer.position!
        else
            ( If the read failed, check to see if we were interrupted by a signal. )
            read-bytes @ -1 =
            posix.errno@ posix.EINTR <>
            &&
            if
                ( Looks like it was some other error. )
                break
            then
        then

        ( Keep reading until we've read the entire buffer or we've reached the end of the file. )
        buffer @ buffer.position@ total-size @ =
        file-fd @ file.is-eof?
        ||
    until

    ( Was the read successful? )
    read-bytes @ -1 =
    if
        file-fd @ posix.errno@ posix.strerror "Unable to read from fd {}: {}." string.format throw
    then

    ( Return the new buffer to the caller. )
    buffer @
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
: file.line@  ( file-id -- string )
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
    variable! file-fd
    variable! buffer

    ( If the value isn't a buffer, convert it to a string and write that string to a new buffer. )
    buffer @ value.is-buffer? '
    if
        ( Make sure that value is a string, and create a new buffer to hold it. )
        buffer @ value.to-string
        dup string.size@ dup buffer.new buffer !

        ( Write the string to the buffer. )
        ( Note, buffer.string! expects it's parameters to be passed as string buffer size. )
        ( string size ) buffer @ swap buffer.string!
    then

    ( Now that value is a byte-buffer, write it to the file.  Keeping in mind that we may be )
    ( interrupted by a signal. )
    buffer @ 0 buffer.position!

    ( Get the total number of bytes we're to write.  Also keep track of how many bytes we've )
    ( written so far. )
    buffer @ buffer.size@ variable! total-size
    0 variable! written-bytes

    ( Attempt to write the buffer to the file, keeping in mind we may be interrupted by a signal )
    ( or that the write may not be able to write the entire buffer in one go. )
    begin
        0 posix.errno!

        ( Write the buffer to the file, and keep track of how many bytes were actually written. )
        file-fd @  buffer @  total-size @ written-bytes @ -  posix.write  written-bytes !

        ( Check to see if the write was successful. )
        written-bytes @ 0>=
        if
            ( Update the buffer's position to reflect the number of bytes written. )
            buffer @  buffer @ buffer.position@ written-bytes @ +  buffer.position!
        else
            ( If the write failed, check to see if we were interrupted by a signal. )
            written-bytes -1 =
            posix.errno@ posix.EINTR <>
            &&
            if
                ( Looks like it was some other error. )
                break
            then
        then

        ( Keep writing until we've written the entire buffer. )
        buffer @ buffer.position@ total-size @ =
    until

    ( Was the write successful. )
    written-bytes @ -1 =
    if
        file-fd @ posix.errno@ posix.strerror "Unable to write to fd {}: {}." string.format throw
    then

    ( Reset the buffer's position to the beginning. )
    buffer @ 0 buffer.position!
;



( Write a string to the file appending a new \n character at the end of the write. )
: file.line!  ( value file-id -- )
    variable! file-fd
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
        value @ value.to-string "\n" + file-fd @ file.!
    then
;



( Read a FD or file path and get the size of the associated file. )
: file.size@  ( file -- size )
    variable! file

    variable result
    variable stat-info

    ( Check which version of stat to call, we should have been passed either a file path or a file )
    ( descriptor. )
    file @ value.is-string?
    if
        file @ posix.stat result ! stat-info !
    else
        file @ value.is-number?
        if
            file @ posix.fstat result ! stat-info !
        else
            "Invalid file descriptor or path." throw
        then
    then

    ( Was the call successful or did it fail? )
    result @ -1 =
    if
        file @ posix.errno@ posix.strerror "Unable to get file size for {}: {}." string.format throw
    then

    ( Return the size of the file. )
    stat-info posix.stat-struct.st_size@@
;



( Check to see if a given file exists. )
: file.exists?  ( path -- exists? )
    variable! path

    ( Call the stat function, but drop the structure result because we don't need it. )
    path @ posix.stat swap drop

    ( Was the call successful or did it fail? )
    -1 =
    if
        ( No file. )
        false
    else
        ( File exists. )
        true
    then
;

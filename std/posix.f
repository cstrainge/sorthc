
( Definitions for the POSIX API. )



( POSIX error codes. )
1   constant posix.EPERM
2   constant posix.ENOENT
3   constant posix.ESRCH
4   constant posix.EINTR
5   constant posix.EIO
6   constant posix.ENXIO
7   constant posix.E2BIG
8   constant posix.ENOEXEC
9   constant posix.EBADF
10  constant posix.ECHILD
11  constant posix.EAGAIN
12  constant posix.ENOMEM
13  constant posix.EACCES
14  constant posix.EFAULT
15  constant posix.ENOTBLK
16  constant posix.EBUSY
17  constant posix.EEXIST
18  constant posix.EXDEV
19  constant posix.ENODEV
20  constant posix.ENOTDIR
21  constant posix.EISDIR
22  constant posix.EINVAL
23  constant posix.ENFILE
24  constant posix.EMFILE
25  constant posix.ENOTTY
26  constant posix.ETXTBSY
27  constant posix.EFBIG
28  constant posix.ENOSPC
29  constant posix.ESPIPE
30  constant posix.EROFS
31  constant posix.EMLINK
32  constant posix.EPIPE
33  constant posix.EDOM
34  constant posix.ERANGE



1 constant posix.AF_UNIX



1 constant posix.SOCK_STREAM



( Define the array of bytes that makes up the sun_path field of the sockaddr_un structure. )
ffi.[] posix.sun_path 108 -> ffi.u8 as ffi.string ;



( The posix sockaddr_un structure as defined in 64-bit Linux. )
ffi.# posix.sockaddr_un  align 2
    ffi.u16 sun_family -> 0 ,
    posix.sun_path sun_path -> ""
;



( The size of the sockaddr_un structure, as defined in 64-bit Linux. )
110 constant posix.sockaddr_un.size@



ffi.fn socket as posix.socket ffi.i32 ffi.i32 ffi.i32 -> ffi.i32

ffi.fn connect as posix.connect ffi.i32 posix.sockaddr_un:ptr ffi.i32 -> ffi.i32



( We rely on the run-time library to define posix.errno@ and posix.errno!.  This is because we )
( don't have a way to deal with functions that return pointers to values, which the errno macro )
( depends on to return a thread local errno value. )



( File permission flags. )
64  constant posix.O_CREAT
512 constant posix.O_TRUNC


( File system permission flags. )
0o4000 constant posix.S_ISUID
0o2000 constant posix.S_ISGID
0o1000 constant posix.S_ISVTX

0o0400 constant posix.S_IRUSR
0o0200 constant posix.S_IWUSR
0o0100 constant posix.S_IXUSR

0o0040 constant posix.S_IRGRP
0o0020 constant posix.S_IWGRP
0o0010 constant posix.S_IXGRP

0o0004 constant posix.S_IROTH
0o0002 constant posix.S_IWOTH
0o0001 constant posix.S_IXOTH



1 constant posix.F_GETFD



( File seeking flags. )
0 constant posix.SEEK_SET
1 constant posix.SEEK_CUR
2 constant posix.SEEK_END



( TODO: Checkout how these structures are defined in macOS. )



( The posix timespec structure as defined in 64-bit Linux. )
ffi.# posix.timespec
    ffi.u64 tv_sec -> 0 ,
    ffi.u64 tv_nsec -> 0
;



( The posix stat structure as defined in 64-bit Linux. )
ffi.# posix.stat-struct
    ffi.u64 st_dev -> 0 ,
    ffi.u64 st_ino -> 0 ,
    ffi.u64 st_nlink -> 0 ,
    ffi.u32 st_mode -> 0 ,
    ffi.u32 st_uid -> 0 ,
    ffi.u32 st_gid -> 0 ,
    ffi.u32 pad.0 -> 0 ,
    ffi.u64 st_rdev -> 0 ,
    ffi.i64 st_size -> 0 ,
    ffi.i64 st_blksize -> 0 ,
    ffi.i64 st_blocks -> 0 ,
    posix.timespec st_atim -> posix.timespec.new ,
    posix.timespec st_mtim -> posix.timespec.new ,
    posix.timespec st_ctim -> posix.timespec.new ,
    ffi.u64 glibc_reserved.1 -> 0 ,
    ffi.u64 glibc_reserved.2 -> 0 ,
    ffi.u64 glibc_reserved.3 -> 0
;



( Import the posix file handling functions. )

ffi.fn open as posix.open ffi.string ffi.i32 ffi.var-arg -> ffi.i32

ffi.fn strerror as posix.strerror ffi.i32 -> ffi.string

ffi.fn fcntl as posix.fcntl ffi.i32 ffi.i32 ffi.var-arg -> ffi.i32

ffi.fn read as posix.read ffi.i32 ffi.void:ptr ffi.u64 -> ffi.i64

ffi.fn write as posix.write ffi.i32 ffi.void:ptr ffi.u64 -> ffi.i64

ffi.fn chmod as posix.chmod ffi.string ffi.u32 -> ffi.i32

ffi.fn fchmod as posix.fchmod ffi.i32 ffi.u32 -> ffi.i32

ffi.fn close as posix.close ffi.i32 -> ffi.i32

ffi.fn lseek as posix.lseek ffi.i32 ffi.i32 ffi.i32 -> ffi.i32

ffi.fn stat as posix.stat ffi.string posix.stat-struct:out.ptr -> ffi.i32

ffi.fn fstat as posix.fstat ffi.i32 posix.stat-struct:out.ptr -> ffi.i32

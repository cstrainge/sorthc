
# A compiler for the Strange Forth Language

This is an experimental ahead of time compiler for the [Strange Forth Language](https://github.com/cstrainge/sorth).  It is still catching up with the features of the interpreted version of the
language however it can handle most of the language features.

This tool will compile your Strange Forth language to an executable.  Your immediate words run
within a limited version of the runtime within the compiler itself.  All other words are compiled
into the final executable.

The compiler has so far only been tested in Linux.  I'm sure that there'll need to be some changes
needed to support macOS and Windows.

An install of the LLVM development libraries is required to build the compiler.

To build the compiler itself, make a build directory and run cmake, for example:

```
$ mkdir build ; cd build
$ cmake .. -G Ninja
```

If the build script was able to find LLVM it should generate a ninja file that you can build using
`ninja`.  The `sorthc` executable will end up being built in the `dist` directory off of the main
directory of the project.

## Example Usage

Assuming `sorthc` is in your path, and you're running out of the base directory:

```
$ sorthc my-code.f my-code.o
```

This will compile your Forth code into an object file.  To create an executable you'll need to link
the object against the run-time library:

```
$ sorthl my-code.o my-code
```

You will get an executable named `my-code` and it's only dependencies will be on the C++ and C
run-time libraries.

# SCL — Secure Computation Library

SCL is a utilities library primarily for prototyping MPC (or SMC) protocols.
The purpose of this library is to provide a collection of utility functionality
that takes away most, or all, of the usually tedious boilerplate code that is
needed whenever some new and exciting MPC protocol is being
implemented. Hopefully, by using SCL, researches (and hobbyists) will find it a
lot easier, and quicker!, to implement MPC protocols.

## Building SCL

SCL has only a single external dependency, namely
[catch2](https://github.com/catchorg/Catch2/tree/v2.x) which is used for
tests. Besides that, SCL has no dependencies and so should be straightforward to
build and use.

Running `make` provides a short list of available targets.

### Debug build

The default target builds SCL in debug mode intended for development and
testing. Running `make debug` will build SCL, test and then run tests.

```
make debug
```

It is also possible to just build the debug build manually by running either

```
make _cmake_debug
```

or

```
cmake -DCMAKE_BUILD_TYPE=Debug -B build_dir
```

which are essentially equivalent.

The Debug build has a `test` and `coverage` target.

### `compile_commands.json`

Some IDEs use a `compile_commands.json` file, which can be buildt with the `make
generate_compile_commands_json` target.

### Release build

To build SCL as a library (i.e., to produce a `libscl.so` file), just run `make
release`. The `.so` file is then located in `lib`.

## Using SCL

To use SCL, link `libscl.so` when building your program and include the
`include/` directory to your builds includes. See
[examples/README.md](https://github.com/anderspkd/secure-computation-library/tree/cleanup/examples/README.md)
for an example of how to use build applications using SCL.

## Documentation

SCL uses Doxygen for documentation. Run `make documentation` to generate the
documentation. This is placed in the `doc/` folder.

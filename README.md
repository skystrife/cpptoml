# cpptoml
A header-only library for parsing [TOML][toml] configuration files.

Targets: [TOML v0.4.0][currver]

It is reasonably conforming, with the exception of unicode escape
characters in strings. This includes support for the new DateTime format,
inline tables, multi-line basic and raw strings, and digit separators.

Alternatives:
- [ctoml][ctoml] and [tinytoml][tinytoml] are both C++11 implementations of
  a TOML parser, but only support v0.2.0.
- [libtoml][libtoml] is a C implementation of a TOML parser, which can be
  linked to from your C++ programs easily. It supports only v0.2.0 also.

## Build Status
[![Build Status](https://travis-ci.org/skystrife/cpptoml.svg?branch=master)](https://travis-ci.org/skystrife/cpptoml)

[![Build Status](https://webapi.biicode.com/v1/badges/skystrife/skystrife/cpptoml/master)](https://www.biicode.com/skystrife/cpptoml)

## Test Results
The following two tests are the only failing tests from [the toml-test
suite][toml-test].

```
Test: string-escapes (valid)

Parsing failed: Invalid escape sequence at line 9

-------------------------------------------------------------------------------
Test: unicode-escape (valid)

Parsing failed: Invalid escape sequence at line 1


76 passed, 2 failed
```

# Compilation
Requires a well conforming C++11 compiler. On OSX this means clang++ with
libc++ and libc++abi (the default clang installed with XCode's command line
tools is sufficient).

On Linux, you should be able to use g++ >= 4.8.x, or clang++ with libc++
and libc++abi (if your package manager supplies this; most don't).

Compiling the examples can be done with cmake:

```
mkdir build
cd build
cmake ../
make
```

Compile it on Linux, OS X and Windows (MinGW 4.9 and Visual Studio 12) with
[biicode][biicode]:

```
bii init -L
bii build
```

# Example Usage
See the root directory files `parse.cpp` and `parse_stdin.cpp` for an
example usage.

# Use it in biicode

``bii init -L``

Fill your ``biicode.conf`` file just like this:

```
[requirements]
    skystrife/cpptoml: 0

[includes]
     cpptoml.h :skystrife/cpptoml/include
```

``bii build`` and you're ready to go! (An example block is available here:
[amalulla/parse-toml](https://www.biicode.com/amalulla/parse-toml))

[currver]: https://github.com/toml-lang/toml/blob/master/versions/en/toml-v0.4.0.md
[toml]: https://github.com/toml-lang/toml
[toml-test]: https://github.com/BurntSushi/toml-test
[ctoml]: https://github.com/evilncrazy/ctoml
[libtoml]: https://github.com/ajwans/libtoml
[tinytoml]: https://github.com/mayah/tinytoml
[biicode]: https://www.biicode.com

# cpptoml
A header-only library for parsing [TOML][toml] configuration files.

It is reasonably conformant, with the exception of unicode characters in
strings.

Alternatives: 
- [ctoml][ctoml] is another C++11 implementation of a TOML
  parser. The approach taken in both for parsing is similar, but the output
  format is dramatically different---ctoml uses a compact `union` approach
  where `cpptoml` uses an inheritance hierarchy with type erasure. This means
  that `cpptoml` will use familiar standard types like `std::string` and
  `std::vector`, but potentially at the expense of runtime performance.
- [libtoml][libtoml] is a C implementation of a TOML parser, which can be
  linked to from your C++ programs easily.

## Test Results
The following two tests are the only failing tests from [the toml-test
suite][toml-test].

```
Test: string-escapes (valid)

Error running test: Could not decode JSON output from parser: invalid character '\f' in string literal
-------------------------------------------------------------------------------
Test: unicode-escape (valid)

Error running test: exit status 1

54 passed, 2 failed
```

# Compilation
Requires a well conforming C++11 compiler. My recommendation is clang++ with
libc++ and libc++abi, as this combination can't really be beat in terms of
completed features + ABI support on Linux and OSX.

Compiling the examples can be done with cmake:

```
mkdir build
cd build
cmake ../ -DCMAKE_BUILD_TYPE=Debug
```

# Example Usage
See the root directory files `parse.cpp` and `parse_stdin.cpp` for an
example usage.

[toml]: https://github.com/mojombo/toml
[toml-test]: https://github.com/BurntSushi/toml-test
[ctoml]: https://github.com/evilncrazy/ctoml
[libtoml]: https://github.com/ajwans/libtoml

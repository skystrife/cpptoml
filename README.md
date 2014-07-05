# cpptoml
A header-only library for parsing [TOML][toml] configuration files.

Targets:
[0.2.0](https://github.com/toml-lang/toml/blob/master/versions/toml-v0.2.0.md)

It is reasonably conforming, with the exception of unicode characters and
escape characters in strings.

Alternatives:
- [ctoml][ctoml] is another C++11 implementation of a TOML
  parser. It used to take a dramatically different approach to
  representation, but over time it has evolved to be similar in structure
  to cpptoml.
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

62 passed, 2 failed
```

# Compilation
Requires a well conforming C++11 compiler. My recommendation is clang++ with
libc++ and libc++abi, as this combination can't really be beat in terms of
completed features + ABI support on Linux and OSX.

Compiling the examples can be done with cmake:

```
mkdir build
cd build
cmake ../
make
```

# Example Usage
See the root directory files `parse.cpp` and `parse_stdin.cpp` for an
example usage.

[toml]: https://github.com/toml-lang/toml
[toml-test]: https://github.com/BurntSushi/toml-test
[ctoml]: https://github.com/evilncrazy/ctoml
[libtoml]: https://github.com/ajwans/libtoml

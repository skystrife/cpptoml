# cpptoml
A header-only library for parsing [TOML][toml] configuration files.

It is reasonably conformant, with the exception of unicode characters in
strings.

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

#include "cpptoml.h"

#include <iostream>
#include <cassert>

int main(int argc, char** argv)
{
    if (argc < 2)
    {
        std::cout << "Usage: " << argv[0] << " filename" << std::endl;
        return 1;
    }
    cpptoml::toml_group g = cpptoml::parse_file(argv[1]);
    std::cout << g << std::endl;
    return 0;
}

#include "cpptoml.h"

#include <iostream>
#include <cassert>

int main( int argc, char ** argv ) {
    if( argc < 2 ) {
        std::cout << "Usage: " << argv[0] << " filename" << std::endl;
        return 1;
    }
    std::ifstream file{ argv[1] };
    cpptoml::parser p{ file };
    cpptoml::toml_group g = p.parse();
    std::cout << g << std::endl;
    return 0;
}

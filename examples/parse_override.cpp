#include "cpptoml.h"

#include <iostream>
#include <cassert>

int main(int argc, char** argv)
{
    if (argc < 3)
    {
        std::cout << "Usage: " << argv[0] << " basefile overridefile" << std::endl;
        return 1;
    }

    try
    {
        std::shared_ptr<cpptoml::table> conf = cpptoml::parse_base_and_override_files(argv[1], argv[2], true);
        std::cout << (*conf) << std::endl;
    }
    catch (const cpptoml::parse_exception& e)
    {
        std::cerr << "Failed to parse " << argv[1] << ": " << e.what() << std::endl;
        return 1;
    }

    return 0;
}

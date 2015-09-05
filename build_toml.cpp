#include <cpptoml.h>
#include <iostream>

int main(int argc, char* argv[])
{
    cpptoml::table root;
    root.insert("Integer", 1234L);
    root.insert("Double", 1.234);
    root.insert("String", std::string("ABCD"));

    auto table = std::make_shared<cpptoml::table>();
    table->insert("ElementOne", 1L);
    table->insert("ElementTwo", 2.0);
    table->insert("ElementThree", std::string("THREE"));

    auto nested_table = std::make_shared<cpptoml::table>();
    nested_table->insert("ElementOne", 2L);
    nested_table->insert("ElementTwo", 3.0);
    nested_table->insert("ElementThree", std::string("FOUR"));

    table->insert("Nested", nested_table);

    root.insert("Table", table);

    auto int_array = std::make_shared<cpptoml::array>();
    int_array->push_back(1L);
    int_array->push_back(2L);
    int_array->push_back(3L);
    int_array->push_back(4L);
    int_array->push_back(5L);

    root.insert("IntegerArray", int_array);

    auto double_array = std::make_shared<cpptoml::array>();
    double_array->push_back(1.1);
    double_array->push_back(2.2);
    double_array->push_back(3.3);
    double_array->push_back(4.4);
    double_array->push_back(5.5);

    root.insert("DoubleArray", double_array);

    auto string_array = std::make_shared<cpptoml::array>();
    string_array->push_back(std::string("A"));
    string_array->push_back(std::string("B"));
    string_array->push_back(std::string("C"));
    string_array->push_back(std::string("D"));
    string_array->push_back(std::string("E"));

    root.insert("StringArray", string_array);

    auto table_array = std::make_shared<cpptoml::table_array>();
    table_array->push_back(table);
    table_array->push_back(table);
    table_array->push_back(table);

    root.insert("TableArray", table_array);

    auto array_of_arrays = std::make_shared<cpptoml::array>();
    array_of_arrays->push_back(int_array);
    array_of_arrays->push_back(double_array);
    array_of_arrays->push_back(string_array);

    root.insert("ArrayOfArrays", array_of_arrays);

    std::cout << root;
    return 0;
}

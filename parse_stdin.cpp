#include "cpptoml.h"

#include <iostream>
#include <limits>

std::string escape_string(const std::string& str)
{
    std::string res;
    for (auto it = str.begin(); it != str.end(); ++it)
    {
        if (*it == '\\')
            res += "\\\\";
        else if (*it == '"')
            res += "\\\"";
        else if (*it == '\n')
            res += "\\n";
        else
            res += *it;
    }
    return res;
}

void print_value(std::ostream& o, const std::shared_ptr<cpptoml::base>& base)
{
    if (auto v = base->as<std::string>())
    {
        o << "{\"type\":\"string\",\"value\":\"" << escape_string(v->get())
          << "\"}";
    }
    else if (auto v = base->as<int64_t>())
    {
        o << "{\"type\":\"integer\",\"value\":\"" << v->get() << "\"}";
    }
    else if (auto v = base->as<double>())
    {
        o << "{\"type\":\"float\",\"value\":\"" << v->get() << "\"}";
    }
    else if (auto v = base->as<cpptoml::datetime>())
    {
        o << "{\"type\":\"datetime\",\"value\":\"" << v->get() << "\"}";
    }
    else if (auto v = base->as<bool>())
    {
        o << "{\"type\":\"bool\",\"value\":\"";
        v->print(o);
        o << "\"}";
    }
}

void print_array(std::ostream& o, cpptoml::array& arr)
{
    o << "{\"type\":\"array\",\"value\":[";
    auto it = arr.get().begin();
    while (it != arr.get().end())
    {
        if ((*it)->is_array())
            print_array(o, *(*it)->as_array());
        else
            print_value(o, *it);

        if (++it != arr.get().end())
            o << ", ";
    }
    o << "]}";
}

void print_table(std::ostream& o, cpptoml::table& g)
{
    o << "{";
    auto it = g.begin();
    while (it != g.end())
    {
        o << '"' << escape_string(it->first) << "\":";
        if (it->second->is_array())
        {
            print_array(o, *it->second->as_array());
        }
        else if (it->second->is_table())
        {
            print_table(o, *g.get_table(it->first));
        }
        else if (it->second->is_table_array())
        {
            o << "[";
            auto arr = g.get_table_array(it->first)->get();
            auto ait = arr.begin();
            while (ait != arr.end())
            {
                print_table(o, **ait);
                if (++ait != arr.end())
                    o << ", ";
            }
            o << "]";
        }
        else
        {
            print_value(o, it->second);
        }
        if (++it != g.end())
            o << ", ";
    }
    o << "}";
}

int main()
{
    std::cout.precision(std::numeric_limits<double>::max_digits10);
    cpptoml::parser p{std::cin};
    try
    {
        cpptoml::table g = p.parse();
        print_table(std::cout, g);
        std::cout << std::endl;
    }
    catch (const cpptoml::parse_exception& ex)
    {
        std::cerr << "Parsing failed: " << ex.what() << std::endl;
        return 1;
    }
    return 0;
}

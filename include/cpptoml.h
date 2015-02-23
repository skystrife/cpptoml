/**
 * @file cpptoml.h
 * @author Chase Geigle
 * @date May 2013
 */

#ifndef _CPPTOML_H_
#define _CPPTOML_H_

#include <algorithm>
#if !CPPTOML_HAS_STD_PUT_TIME
#include <array>
#endif
#include <cassert>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <memory>
#if CPPTOML_HAS_STD_REGEX
#include <regex>
#endif
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace cpptoml
{

template <class T>
class option
{
  public:
    option() : empty_{true}
    {
        // nothing
    }

    option(T value) : empty_{false}, value_{std::move(value)}
    {
        // nothing
    }

    explicit operator bool() const
    {
        return !empty_;
    }

    const T& operator*() const
    {
        return value_;
    }

  private:
    bool empty_;
    T value_;
};

struct datetime
{
    int year = 0;
    int month = 0;
    int day = 0;
    int hour = 0;
    int minute = 0;
    int second = 0;
    int microsecond = 0;
    int hour_offset = 0;
    int minute_offset = 0;
};

inline std::ostream& operator<<(std::ostream& os, const datetime& dt)
{
    using std::setw;
    auto fill = os.fill();

    os.fill('0');
    os << setw(4) << dt.year << "-" << setw(2) << dt.month << "-" << setw(2)
       << dt.day << "T" << setw(2) << dt.hour << ":" << setw(2) << dt.minute
       << ":" << setw(2) << dt.second;

    if (dt.microsecond > 0)
    {
        os << "." << setw(6) << dt.microsecond;
    }

    if (dt.hour_offset != 0 || dt.minute_offset != 0)
    {
        if (dt.hour_offset > 0)
            os << "+";
        else
            os << "-";
        os << setw(2) << std::abs(dt.hour_offset) << ":" << setw(2)
           << std::abs(dt.minute_offset);
    }
    else
        os << "Z";

    os.fill(fill);

    return os;
}

template <class T>
class value;

class array;
class table;
class table_array;

/**
 * A generic base TOML value used for type erasure.
 */
class base : public std::enable_shared_from_this<base>
{
  public:
    /**
     * Determines if the given TOML element is a value.
     */
    virtual bool is_value() const
    {
        return false;
    }

    /**
     * Determines if the given TOML element is a table.
     */
    virtual bool is_table() const
    {
        return false;
    }

    /**
     * Converts the TOML element into a table.
     */
    std::shared_ptr<table> as_table()
    {
        if (is_table())
            return std::static_pointer_cast<table>(shared_from_this());
        return nullptr;
    }
    /**
     * Determines if the TOML element is an array of "leaf" elements.
     */
    virtual bool is_array() const
    {
        return false;
    }

    /**
     * Converts the TOML element to an array.
     */
    std::shared_ptr<array> as_array()
    {
        if (is_array())
            return std::static_pointer_cast<array>(shared_from_this());
        return nullptr;
    }

    /**
     * Determines if the given TOML element is an array of tables.
     */
    virtual bool is_table_array() const
    {
        return false;
    }

    /**
     * Converts the TOML element into a table array.
     */
    std::shared_ptr<table_array> as_table_array()
    {
        if (is_table_array())
            return std::static_pointer_cast<table_array>(shared_from_this());
        return nullptr;
    }

    /**
     * Prints the TOML element to the given stream.
     */
    virtual void print(std::ostream& stream) const = 0;

    /**
     * Attempts to coerce the TOML element into a concrete TOML value
     * of type T.
     */
    template <class T>
    std::shared_ptr<value<T>> as();
};

template <class T>
struct valid_value
{
    const static bool value
        = std::is_same<T, std::string>::value || std::is_same<T, int64_t>::value
          || std::is_same<T, double>::value || std::is_same<T, bool>::value
          || std::is_same<T, datetime>::value;
};

/**
 * A concrete TOML value representing the "leaves" of the "tree".
 */
template <class T>
class value : public base
{
  public:
    static_assert(valid_value<T>::value, "invalid value type");

    /**
     * Constructs a value from the given data.
     */
    value(const T& val) : data_(val)
    {
    }

    bool is_value() const override
    {
        return true;
    }

    /**
     * Gets the data associated with this value.
     */
    T& get()
    {
        return data_;
    }

    /**
     * Gets the data associated with this value. Const version.
     */
    const T& get() const
    {
        return data_;
    }

    void print(std::ostream& stream) const override
    {
        stream << data_;
    }

  private:
    T data_;
};

// specializations for printing nicely
template <>
inline void value<bool>::print(std::ostream& stream) const
{
    if (data_)
        stream << "true";
    else
        stream << "false";
}

template <class T>
inline std::shared_ptr<value<T>> base::as()
{
    if (auto v = std::dynamic_pointer_cast<value<T>>(shared_from_this()))
        return v;
    return nullptr;
}

class array : public base
{
  public:
    array() = default;

    template <class InputIterator>
    array(InputIterator begin, InputIterator end)
        : values_{begin, end}
    {
        // nothing
    }

    virtual bool is_array() const override
    {
        return true;
    }

    /**
     * Obtains the array (vector) of base values.
     */
    std::vector<std::shared_ptr<base>>& get()
    {
        return values_;
    }

    /**
     * Obtains the array (vector) of base values. Const version.
     */
    const std::vector<std::shared_ptr<base>>& get() const
    {
        return values_;
    }

    std::shared_ptr<base> at(size_t idx) const
    {
        return values_.at(idx);
    }

    /**
     * Obtains an array of value<T>s. Note that elements may be
     * nullptr if they cannot be converted to a value<T>.
     */
    template <class T>
    std::vector<std::shared_ptr<value<T>>> array_of() const
    {
        std::vector<std::shared_ptr<value<T>>> result(values_.size());

        std::transform(values_.begin(), values_.end(), result.begin(),
                       [&](std::shared_ptr<base> v)
                       {
            return v->as<T>();
        });

        return result;
    }

    /**
     * Obtains an array of arrays. Note that elements may be nullptr
     * if they cannot be converted to a array.
     */
    std::vector<std::shared_ptr<array>> nested_array() const
    {
        std::vector<std::shared_ptr<array>> result(values_.size());

        std::transform(values_.begin(), values_.end(), result.begin(),
                       [&](std::shared_ptr<base> v)
                       {
            if (v->is_array())
                return std::static_pointer_cast<array>(v);
            return std::shared_ptr<array>{};
        });

        return result;
    }

    virtual void print(std::ostream& stream) const override
    {
        stream << "[ ";
        auto it = values_.begin();
        while (it != values_.end())
        {
            (*it)->print(stream);
            if (++it != values_.end())
                stream << ", ";
        }
        stream << " ]";
    }

  private:
    std::vector<std::shared_ptr<base>> values_;
};

class table;

class table_array : public base
{
    friend class table;

  public:
    virtual bool is_table_array() const override
    {
        return true;
    }

    std::vector<std::shared_ptr<table>>& get()
    {
        return array_;
    }

    void print(std::ostream& stream) const override
    {
        print(stream, 0, "");
    }

  private:
    void print(std::ostream& stream, size_t depth,
               const std::string& key) const;
    std::vector<std::shared_ptr<table>> array_;
};

/**
 * Represents a TOML keytable.
 */
class table : public base
{
  public:
    friend class table_array;
    /**
     * tables can be iterated over.
     */
    using iterator
        = std::unordered_map<std::string, std::shared_ptr<base>>::iterator;

    /**
     * tables can be iterated over. Const version.
     */
    using const_iterator
        = std::unordered_map<std::string,
                             std::shared_ptr<base>>::const_iterator;

    iterator begin()
    {
        return map_.begin();
    }

    const_iterator begin() const
    {
        return map_.begin();
    }

    iterator end()
    {
        return map_.end();
    }

    const_iterator end() const
    {
        return map_.end();
    }

    bool is_table() const override
    {
        return true;
    }

    bool empty() const
    {
        return map_.empty();
    }

    /**
     * Determines if this key table contains the given key.
     */
    bool contains(const std::string& key) const
    {
        return map_.find(key) != map_.end();
    }

    /**
     * Determines if this key table contains the given key. Will
     * resolve "qualified keys". Qualified keys are the full access
     * path separated with dots like "grandparent.parent.child".
     */
    bool contains_qualified(const std::string& key) const
    {
        return resolve_qualified(key);
    }

    /**
     * Obtains the base for a given key.
     * @throw std::out_of_range if the key does not exist
     */
    std::shared_ptr<base> get(const std::string& key) const
    {
        return map_.at(key);
    }

    /**
     * Obtains the base for a given key. Will resolve "qualified
     * keys". Qualified keys are the full access path separated with
     * dots like "grandparent.parent.child".
     *
     * @throw std::out_of_range if the key does not exist
     */
    std::shared_ptr<base> get_qualified(const std::string& key) const
    {
        std::shared_ptr<base> p;
        resolve_qualified(key, &p);
        return p;
    }

    /**
     * Obtains a table for a given key, if possible.
     */
    std::shared_ptr<table> get_table(const std::string& key) const
    {
        if (contains(key) && get(key)->is_table())
            return std::static_pointer_cast<table>(get(key));
        return nullptr;
    }

    /**
     * Obtains a table for a given key, if possible. Will resolve
     * "qualified keys".
     */
    std::shared_ptr<table> get_table_qualified(const std::string& key) const
    {
        if (contains_qualified(key) && get_qualified(key)->is_table())
            return std::static_pointer_cast<table>(get_qualified(key));
        return nullptr;
    }

    /**
     * Obtains an array for a given key.
     */
    std::shared_ptr<array> get_array(const std::string& key) const
    {
        if (!contains(key))
            return nullptr;
        return get(key)->as_array();
    }

    /**
     * Obtains an array for a given key. Will resolve "qualified keys".
     */
    std::shared_ptr<array> get_array_qualified(const std::string& key) const
    {
        if (!contains_qualified(key))
            return nullptr;
        return get_qualified(key)->as_array();
    }

    /**
     * Obtains a table_array for a given key, if possible.
     */
    std::shared_ptr<table_array> get_table_array(const std::string& key) const
    {
        if (!contains(key))
            return nullptr;
        return get(key)->as_table_array();
    }

    /**
     * Obtains a table_array for a given key, if possible. Will resolve
     * "qualified keys".
     */
    std::shared_ptr<table_array>
        get_table_array_qualified(const std::string& key) const
    {
        if (!contains_qualified(key))
            return nullptr;
        return get_qualified(key)->as_table_array();
    }

    /**
     * Helper function that attempts to get a value corresponding
     * to the template parameter from a given key.
     */
    template <class T>
    option<T> get_as(const std::string& key) const
    {
        try
        {
            if (auto v = get(key)->as<T>())
                return {v->get()};
            else
                return {};
        }
        catch (const std::out_of_range&)
        {
            return {};
        }
    }

    /**
     * Helper function that attempts to get a value corresponding
     * to the template parameter from a given key. Will resolve "qualified
     * keys".
     */
    template <class T>
    option<T> get_qualified_as(const std::string& key) const
    {
        try
        {
            if (auto v = get_qualified(key)->as<T>())
                return {v->get()};
            else
                return {};
        }
        catch (const std::out_of_range&)
        {
            return {};
        }
    }

    /**
     * Adds an element to the keytable.
     */
    void insert(const std::string& key, const std::shared_ptr<base>& value)
    {
        map_[key] = value;
    }

    /**
     * Convenience shorthand for adding a simple element to the
     * keytable.
     */
    template <class T>
    void insert(const std::string& key, T&& val,
                typename std::enable_if<valid_value<T>::value>::type* = 0)
    {
        insert(key, std::make_shared<value<T>>(val));
    }

    friend std::ostream& operator<<(std::ostream& stream, const table& table);

    void print(std::ostream& stream) const override
    {
        print(stream, 0);
    }

  private:
    std::vector<std::string> split(const std::string& value,
                                   char separator) const
    {
        std::vector<std::string> result;
        std::string::size_type p = 0;
        std::string::size_type q;
        while ((q = value.find(separator, p)) != std::string::npos)
        {
            result.emplace_back(value, p, q - p);
            p = q + 1;
        }
        result.emplace_back(value, p);
        return result;
    }

    // If output parameter p is specified, fill it with the pointer to the
    // specified entry and throw std::out_of_range if it couldn't be found.
    //
    // Otherwise, just return true if the entry could be found or false
    // otherwise and do not throw.
    bool resolve_qualified(const std::string& key,
                           std::shared_ptr<base>* p = nullptr) const
    {
        auto parts = split(key, '.');
        auto last_key = parts.back();
        parts.pop_back();

        auto table = this;
        for (const auto& part : parts)
        {
            table = table->get_table(part).get();
            if (!table)
            {
                if (!p)
                    return false;

                throw std::out_of_range{key + " is not a valid key"};
            }
        }

        if (!p)
            return table->map_.count(last_key) != 0;

        *p = table->map_.at(last_key);
        return true;
    }

    void print(std::ostream& stream, size_t depth) const
    {
        for (auto& p : map_)
        {
            if (p.second->is_table_array())
            {
                auto ga = std::dynamic_pointer_cast<table_array>(p.second);
                ga->print(stream, depth, p.first);
            }
            else
            {
                stream << std::string(depth, '\t') << p.first << " = ";
                if (p.second->is_table())
                {
                    auto g = static_cast<table*>(p.second.get());
                    stream << '\n';
                    g->print(stream, depth + 1);
                }
                else
                {
                    p.second->print(stream);
                    stream << '\n';
                }
            }
        }
    }
    std::unordered_map<std::string, std::shared_ptr<base>> map_;
};

inline void table_array::print(std::ostream& stream, size_t depth,
                               const std::string& key) const
{
    for (auto g : array_)
    {
        stream << std::string(depth, '\t') << "[[" << key << "]]\n";
        g->print(stream, depth + 1);
    }
}

inline std::ostream& operator<<(std::ostream& stream, const table& table)
{
    table.print(stream);
    return stream;
}

/**
 * Exception class for all TOML parsing errors.
 */
class parse_exception : public std::runtime_error
{
  public:
    parse_exception(const std::string& err) : std::runtime_error{err}
    {
    }

    parse_exception(const std::string& err, std::size_t line_number)
        : std::runtime_error{err + " at line " + std::to_string(line_number)}
    {
    }
};

/**
 * The parser class.
 */
class parser
{
  public:
    /**
     * Parsers are constructed from streams.
     */
    parser(std::istream& stream) : input_(stream)
    {
    }

    parser& operator=(const parser& parser) = delete;

    /**
     * Parses the stream this parser was created on until EOF.
     * @throw parse_exception if there are errors in parsing
     */
    table parse()
    {
        table root;

        table* curr_table = &root;

        while (std::getline(input_, line_))
        {
            line_number_++;
            auto it = line_.begin();
            auto end = line_.end();
            consume_whitespace(it, end);
            if (it == end || *it == '#')
                continue;
            if (*it == '[')
            {
                curr_table = &root;
                parse_table(it, end, curr_table);
            }
            else
            {
                parse_key_value(it, end, curr_table);
                consume_whitespace(it, end);
                eol_or_comment(it, end);
            }
        }
        return root;
    }

  private:
#if defined _MSC_VER
    __declspec(noreturn)
#elif defined __GNUC__
    __attribute__((noreturn))
#endif
        void throw_parse_exception(const std::string& err)
    {
        throw parse_exception{err, line_number_};
    }

    void parse_table(std::string::iterator& it,
                     const std::string::iterator& end, table*& curr_table)
    {
        // remove the beginning keytable marker
        ++it;
        if (it == end)
            throw_parse_exception("Unexpected end of table");
        if (*it == '[')
            parse_table_array(it, end, curr_table);
        else
            parse_single_table(it, end, curr_table);
    }

    void parse_single_table(std::string::iterator& it,
                            const std::string::iterator& end,
                            table*& curr_table)
    {
        if (it == end || *it == ']')
            throw_parse_exception("Table name cannot be empty");

        std::string full_table_name;
        bool inserted = false;
        while (it != end && *it != ']')
        {
            auto part = parse_key(it, end, [](char c)
                                  {
                return c == '.' || c == ']';
            });

            if (part.empty())
                throw_parse_exception("Empty component of table name");

            if (!full_table_name.empty())
                full_table_name += ".";
            full_table_name += part;

            if (curr_table->contains(part))
            {
                auto b = curr_table->get(part);
                if (b->is_table())
                    curr_table = static_cast<table*>(b.get());
                else if (b->is_table_array())
                    curr_table = std::static_pointer_cast<table_array>(b)
                                     ->get()
                                     .back()
                                     .get();
                else
                    throw_parse_exception("Key " + full_table_name
                                          + "already exists as a value");
            }
            else
            {
                inserted = true;
                curr_table->insert(part, std::make_shared<table>());
                curr_table = static_cast<table*>(curr_table->get(part).get());
            }
            consume_whitespace(it, end);
            if (it != end && *it == '.')
                ++it;
            consume_whitespace(it, end);
        }

        // table already existed
        if (!inserted)
        {
            auto is_value = [](const std::pair<const std::string&,
                                               const std::shared_ptr<base>&>& p)
            {
                return p.second->is_value();
            };

            // if there are any values, we can't add values to this table
            // since it has already been defined. If there aren't any
            // values, then it was implicitly created by something like
            // [a.b]
            if (curr_table->empty() || std::any_of(curr_table->begin(),
                                                   curr_table->end(), is_value))
            {
                throw_parse_exception("Redefinition of table "
                                      + full_table_name);
            }
        }

        ++it;
        consume_whitespace(it, end);
        eol_or_comment(it, end);
    }

    void parse_table_array(std::string::iterator& it,
                           const std::string::iterator& end, table*& curr_table)
    {
        ++it;
        if (it == end || *it == ']')
            throw_parse_exception("Table array name cannot be empty");

        std::string full_ta_name;
        while (it != end && *it != ']')
        {
            auto part = parse_key(it, end, [](char c)
                                  {
                return c == '.' || c == ']';
            });

            if (part.empty())
                throw_parse_exception("Empty component of table array name");

            if (!full_ta_name.empty())
                full_ta_name += ".";
            full_ta_name += part;

            consume_whitespace(it, end);
            if (it != end && *it == '.')
                ++it;
            consume_whitespace(it, end);

            if (curr_table->contains(part))
            {
                auto b = curr_table->get(part);

                // if this is the end of the table array name, add an
                // element to the table array that we just looked up
                if (it != end && *it == ']')
                {
                    if (!b->is_table_array())
                        throw_parse_exception("Key " + full_ta_name
                                              + " is not a table array");
                    auto v = b->as_table_array();
                    v->get().push_back(std::make_shared<table>());
                    curr_table = v->get().back().get();
                }
                // otherwise, just keep traversing down the key name
                else
                {
                    if (b->is_table())
                        curr_table = static_cast<table*>(b.get());
                    else if (b->is_table_array())
                        curr_table = std::static_pointer_cast<table_array>(b)
                                         ->get()
                                         .back()
                                         .get();
                    else
                        throw_parse_exception("Key " + full_ta_name
                                              + " already exists as a value");
                }
            }
            else
            {
                // if this is the end of the table array name, add a new
                // table array and a new table inside that array for us to
                // add keys to next
                if (it != end && *it == ']')
                {
                    curr_table->insert(part, std::make_shared<table_array>());
                    auto arr = std::static_pointer_cast<table_array>(
                        curr_table->get(part));
                    arr->get().push_back(std::make_shared<table>());
                    curr_table = arr->get().back().get();
                }
                // otherwise, create the implicitly defined table and move
                // down to it
                else
                {
                    curr_table->insert(part, std::make_shared<table>());
                    curr_table
                        = static_cast<table*>(curr_table->get(part).get());
                }
            }
        }

        // consume the last "]]"
        if (it == end)
            throw_parse_exception("Unterminated table array name");
        ++it;
        if (it == end)
            throw_parse_exception("Unterminated table array name");
        ++it;

        consume_whitespace(it, end);
        eol_or_comment(it, end);
    }

    void parse_key_value(std::string::iterator& it, std::string::iterator& end,
                         table* curr_table)
    {
        auto key = parse_key(it, end, [](char c)
                             {
            return c == '=';
        });
        if (curr_table->contains(key))
            throw_parse_exception("Key " + key + " already present");
        if (*it != '=')
            throw_parse_exception("Value must follow after a '='");
        ++it;
        consume_whitespace(it, end);
        curr_table->insert(key, parse_value(it, end));
        consume_whitespace(it, end);
    }

    template <class Function>
    std::string parse_key(std::string::iterator& it,
                          const std::string::iterator& end, Function&& fun)
    {
        consume_whitespace(it, end);
        if (*it == '"')
        {
            return parse_quoted_key(it, end);
        }
        else
        {
            auto bke = std::find_if(it, end, std::forward<Function>(fun));
            return parse_bare_key(it, bke);
        }
    }

    std::string parse_bare_key(std::string::iterator& it,
                               const std::string::iterator& end)
    {
        auto key_end = end;
        --key_end;
        consume_backwards_whitespace(key_end, it);
        ++key_end;
        std::string key{it, key_end};

        if (std::find(it, key_end, '#') != key_end)
        {
            throw_parse_exception("Bare key " + key + " cannot contain #");
        }

        if (std::find_if(it, key_end, [](char c)
                         {
                return c == ' ' || c == '\t';
            }) != key_end)
        {
            throw_parse_exception("Bare key " + key
                                  + " cannot contain whitespace");
        }

        if (std::find_if(it, key_end, [](char c)
                         {
                return c == '[' || c == ']';
            }) != key_end)
        {
            throw_parse_exception("Bare key " + key
                                  + " cannot contain '[' or ']'");
        }

        it = end;
        return key;
    }

    std::string parse_quoted_key(std::string::iterator& it,
                                 const std::string::iterator& end)
    {
        return string_literal(it, end, '"');
    }

    enum class parse_type
    {
        STRING = 1,
        DATE,
        INT,
        FLOAT,
        BOOL,
        ARRAY,
        INLINE_TABLE
    };

    std::shared_ptr<base> parse_value(std::string::iterator& it,
                                      std::string::iterator& end)
    {
        parse_type type = determine_value_type(it, end);
        switch (type)
        {
            case parse_type::STRING:
                return parse_string(it, end);
            case parse_type::DATE:
                return parse_date(it, end);
            case parse_type::INT:
            case parse_type::FLOAT:
                return parse_number(it, end);
            case parse_type::BOOL:
                return parse_bool(it, end);
            case parse_type::ARRAY:
                return parse_array(it, end);
            case parse_type::INLINE_TABLE:
                return parse_inline_table(it, end);
            default:
                throw_parse_exception("Failed to parse value");
        }
    }

    parse_type determine_value_type(const std::string::iterator& it,
                                    const std::string::iterator& end)
    {
        if (*it == '"' || *it == '\'')
        {
            return parse_type::STRING;
        }
        else if (is_date(it, end))
        {
            return parse_type::DATE;
        }
        else if (is_number(*it) || *it == '-' || *it == '+')
        {
            return determine_number_type(it, end);
        }
        else if (*it == 't' || *it == 'f')
        {
            return parse_type::BOOL;
        }
        else if (*it == '[')
        {
            return parse_type::ARRAY;
        }
        else if (*it == '{')
        {
            return parse_type::INLINE_TABLE;
        }
        throw_parse_exception("Failed to parse value type");
    }

    parse_type determine_number_type(const std::string::iterator& it,
                                     const std::string::iterator& end)
    {
        // determine if we are an integer or a float
        auto check_it = it;
        if (*check_it == '-' || *check_it == '+')
            ++check_it;
        while (check_it != end && is_number(*check_it))
            ++check_it;
        if (check_it != end && *check_it == '.')
        {
            ++check_it;
            while (check_it != end && is_number(*check_it))
                ++check_it;
            return parse_type::FLOAT;
        }
        else
        {
            return parse_type::INT;
        }
    }

    std::shared_ptr<value<std::string>> parse_string(std::string::iterator& it,
                                                     std::string::iterator& end)
    {
        auto delim = *it;
        assert(delim == '"' || delim == '\'');

        // end is non-const here because we have to be able to potentially
        // parse multiple lines in a string, not just one
        auto check_it = it;
        ++check_it;
        if (check_it != end && *check_it == delim)
        {
            ++check_it;
            if (check_it != end && *check_it == delim)
            {
                it = ++check_it;
                return parse_multiline_string(it, end, delim);
            }
        }
        return std::make_shared<value<std::string>>(
            string_literal(it, end, delim));
    }

    std::shared_ptr<value<std::string>>
        parse_multiline_string(std::string::iterator& it,
                               std::string::iterator& end, char delim)
    {
        std::stringstream ss;

        auto is_ws = [](char c)
        {
            return c == ' ' || c == '\t';
        };

        bool consuming = false;
        std::shared_ptr<value<std::string>> ret;

        auto handle_line =
            [&](std::string::iterator& it, std::string::iterator& end)
        {
            if (consuming)
            {
                it = std::find_if_not(it, end, is_ws);

                // whole line is whitespace
                if (it == end)
                    return;
            }

            consuming = false;

            while (it != end)
            {
                auto check = it;
                // handle escaped characters
                if (delim == '"' && *it == '\\')
                {
                    // check if this is an actual escape sequence or a
                    // whitespace escaping backslash
                    ++check;
                    if (check == end)
                    {
                        consuming = true;
                        break;
                    }

                    ss << parse_escape_code(it, end);
                    continue;
                }

                // if we can end the string
                if (std::distance(it, end) >= 3)
                {
                    auto check = it;
                    // check for """
                    if (*check++ == delim && *check++ == delim
                        && *check++ == delim)
                    {
                        it = check;
                        ret = std::make_shared<value<std::string>>(ss.str());
                        break;
                    }
                }

                ss << *it++;
            }
        };

        // handle the remainder of the current line
        handle_line(it, end);
        if (ret)
            return ret;

        // start eating lines
        while (std::getline(input_, line_))
        {
            ++line_number_;

            it = line_.begin();
            end = line_.end();

            handle_line(it, end);

            if (ret)
                return ret;

            if (!consuming)
                ss << std::endl;
        }

        throw_parse_exception("Unterminated multi-line basic string");
    }

    std::string string_literal(std::string::iterator& it,
                               const std::string::iterator& end, char delim)
    {
        ++it;
        std::string val;
        while (it != end)
        {
            // handle escaped characters
            if (delim == '"' && *it == '\\')
            {
                val += parse_escape_code(it, end);
            }
            else if (*it == delim)
            {
                ++it;
                consume_whitespace(it, end);
                return val;
            }
            else
            {
                val += *it++;
            }
        }
        throw_parse_exception("Unterminated string literal");
    }

    char parse_escape_code(std::string::iterator& it,
                           const std::string::iterator& end)
    {
        ++it;
        if (it == end)
            throw_parse_exception("Invalid escape sequence");
        char value;
        if (*it == 'b')
        {
            value = '\b';
        }
        else if (*it == 't')
        {
            value = '\t';
        }
        else if (*it == 'n')
        {
            value = '\n';
        }
        else if (*it == 'f')
        {
            value = '\f';
        }
        else if (*it == 'r')
        {
            value = '\r';
        }
        else if (*it == '"')
        {
            value = '"';
        }
        else if (*it == '\\')
        {
            value = '\\';
        }
        else
        {
            throw_parse_exception("Invalid escape sequence");
        }
        ++it;
        return value;
    }

    std::shared_ptr<base> parse_number(std::string::iterator& it,
                                       const std::string::iterator& end)
    {
        // determine if we are an integer or a float
        auto check_it = it;

        auto eat_sign = [&]()
        {
            if (*check_it == '-' || *check_it == '+')
                ++check_it;
        };

        eat_sign();

        auto eat_numbers = [&]()
        {
            auto beg = check_it;
            while (check_it != end && is_number(*check_it))
            {
                ++check_it;
                if (check_it != end && *check_it == '_')
                {
                    ++check_it;
                    if (check_it == end || !is_number(*check_it))
                        throw_parse_exception("Malformed number");
                }
            }

            if (check_it == beg)
                throw_parse_exception("Malformed number");
        };

        eat_numbers();

        if (check_it != end
            && (*check_it == '.' || *check_it == 'e' || *check_it == 'E'))
        {
            bool is_exp = *check_it == 'e' || *check_it == 'E';

            ++check_it;
            if (check_it == end)
                throw_parse_exception("Floats must have trailing digits");

            if (is_exp)
                eat_sign();

            eat_numbers();

            if (!is_exp && (*check_it == 'e' || *check_it == 'E'))
            {
                ++check_it;
                eat_sign();
                eat_numbers();
            }

            return parse_float(it, check_it);
        }
        else
        {
            return parse_int(it, check_it);
        }
    }

    std::shared_ptr<value<int64_t>> parse_int(std::string::iterator& it,
                                              const std::string::iterator& end)
    {
        std::string v{it, end};
        v.erase(std::remove(v.begin(), v.end(), '_'), v.end());
        it = end;
        try
        {
            return std::make_shared<value<int64_t>>(std::stoll(v));
        }
        catch (const std::invalid_argument& ex)
        {
            throw_parse_exception("Malformed number (invalid argument: "
                                  + std::string{ex.what()} + ")");
        }
        catch (const std::out_of_range& ex)
        {
            throw_parse_exception("Malformed number (out of range: "
                                  + std::string{ex.what()} + ")");
        }
    }

    std::shared_ptr<value<double>> parse_float(std::string::iterator& it,
                                               const std::string::iterator& end)
    {
        std::string v{it, end};
        v.erase(std::remove(v.begin(), v.end(), '_'), v.end());
        it = end;
        try
        {
            return std::make_shared<value<double>>(std::stold(v));
        }
        catch (const std::invalid_argument& ex)
        {
            throw_parse_exception("Malformed number (invalid argument: "
                                  + std::string{ex.what()} + ")");
        }
        catch (const std::out_of_range& ex)
        {
            throw_parse_exception("Malformed number (out of range: "
                                  + std::string{ex.what()} + ")");
        }
    }

    std::shared_ptr<value<bool>> parse_bool(std::string::iterator& it,
                                            const std::string::iterator& end)
    {
        auto boolend = std::find_if(it, end, [](char c)
                                    {
            return c == ' ' || c == '\t' || c == '#';
        });
        std::string v{it, boolend};
        it = boolend;
        if (v == "true")
            return std::make_shared<value<bool>>(true);
        else if (v == "false")
            return std::make_shared<value<bool>>(false);
        else
            throw_parse_exception("Attempted to parse invalid boolean value");
    }

    std::string::iterator find_end_of_date(std::string::iterator it,
                                           std::string::iterator end)
    {
        return std::find_if(it, end, [this](char c)
                            {
            return !is_number(c) && c != 'T' && c != 'Z' && c != ':' && c != '-'
                   && c != '+' && c != '.';
        });
    }

    std::shared_ptr<value<datetime>>
        parse_date(std::string::iterator& it, const std::string::iterator& end)
    {
        auto date_end = find_end_of_date(it, end);

        auto eat = [&](char c)
        {
            if (it == date_end || *it != c)
                throw_parse_exception("Malformed date");
            ++it;
        };

        auto eat_digits = [&](int len)
        {
            int val = 0;
            for (int i = 0; i < len; ++i)
            {
                if (!is_number(*it) || it == date_end)
                    throw_parse_exception("Malformed date");
                val = 10 * val + (*it++ - '0');
            }
            return val;
        };

        datetime dt;

        dt.year = eat_digits(4);
        eat('-');
        dt.month = eat_digits(2);
        eat('-');
        dt.day = eat_digits(2);
        eat('T');
        dt.hour = eat_digits(2);
        eat(':');
        dt.minute = eat_digits(2);
        eat(':');
        dt.second = eat_digits(2);

        if (*it == '.')
        {
            ++it;
            while (it != date_end && is_number(*it))
                dt.microsecond = 10 * dt.microsecond + (*it++ - '0');
        }

        if (it == date_end)
            throw_parse_exception("Malformed date");

        int hoff = 0;
        int moff = 0;
        if (*it == '+' || *it == '-')
        {
            auto plus = *it == '+';
            ++it;

            hoff = eat_digits(2);
            dt.hour_offset = (plus) ? hoff : -hoff;
            eat(':');
            moff = eat_digits(2);
            dt.minute_offset = (plus) ? moff : -moff;
        }
        else if (*it == 'Z')
        {
            ++it;
        }

        if (it != date_end)
            throw_parse_exception("Malformed date");

        return std::make_shared<value<datetime>>(dt);
    }

    std::shared_ptr<base> parse_array(std::string::iterator& it,
                                      std::string::iterator& end)
    {
        // this gets ugly because of the "homogeneity" restriction:
        // arrays can either be of only one type, or contain arrays
        // (each of those arrays could be of different types, though)
        //
        // because of the latter portion, we don't really have a choice
        // but to represent them as arrays of base values...
        ++it;

        // ugh---have to read the first value to determine array type...
        skip_whitespace_and_comments(it, end);

        // edge case---empty array
        if (*it == ']')
        {
            ++it;
            return std::make_shared<array>();
        }

        auto val_end = std::find_if(it, end, [](char c)
                                    {
            return c == ',' || c == ']' || c == '#';
        });
        parse_type type = determine_value_type(it, val_end);
        switch (type)
        {
            case parse_type::STRING:
                return parse_value_array<std::string>(it, end);
            case parse_type::INT:
                return parse_value_array<int64_t>(it, end);
            case parse_type::FLOAT:
                return parse_value_array<double>(it, end);
            case parse_type::DATE:
                return parse_value_array<datetime>(it, end);
            case parse_type::ARRAY:
                return parse_object_array<array>(&parser::parse_array, '[', it,
                                                 end);
            case parse_type::INLINE_TABLE:
                return parse_object_array<table_array>(
                    &parser::parse_inline_table, '{', it, end);
            default:
                throw_parse_exception("Unable to parse array");
        }
    }

    template <class Value>
    std::shared_ptr<array> parse_value_array(std::string::iterator& it,
                                             std::string::iterator& end)
    {
        auto arr = std::make_shared<array>();
        while (it != end && *it != ']')
        {
            auto value = parse_value(it, end);
            if (auto v = value->as<Value>())
                arr->get().push_back(value);
            else
                throw_parse_exception("Arrays must be heterogeneous");
            skip_whitespace_and_comments(it, end);
            if (*it != ',')
                break;
            ++it;
            skip_whitespace_and_comments(it, end);
        }
        if (it != end)
            ++it;
        return arr;
    }

    template <class Object, class Function>
    std::shared_ptr<Object> parse_object_array(Function&& fun, char delim,
                                               std::string::iterator& it,
                                               std::string::iterator& end)
    {
        auto arr = std::make_shared<Object>();

        while (it != end && *it != ']')
        {
            if (*it != delim)
                throw_parse_exception("Unexpected character in array");

            arr->get().push_back(((*this).*fun)(it, end));
            skip_whitespace_and_comments(it, end);

            if (*it != ',')
                break;

            ++it;
            skip_whitespace_and_comments(it, end);
        }

        if (it == end || *it != ']')
            throw_parse_exception("Unterminated array");

        ++it;
        return arr;
    }

    std::shared_ptr<table> parse_inline_table(std::string::iterator& it,
                                              std::string::iterator& end)
    {
        auto tbl = std::make_shared<table>();
        do
        {
            ++it;
            if (it == end)
                throw_parse_exception("Unterminated inline table");

            consume_whitespace(it, end);
            parse_key_value(it, end, tbl.get());
            consume_whitespace(it, end);
        } while (*it == ',');

        if (it == end || *it != '}')
            throw_parse_exception("Unterminated inline table");

        ++it;
        consume_whitespace(it, end);

        return tbl;
    }

    void skip_whitespace_and_comments(std::string::iterator& start,
                                      std::string::iterator& end)
    {
        consume_whitespace(start, end);
        while (start == end || *start == '#')
        {
            if (!std::getline(input_, line_))
                throw_parse_exception("Unclosed array");
            line_number_++;
            start = line_.begin();
            end = line_.end();
            consume_whitespace(start, end);
        }
    }

    void consume_whitespace(std::string::iterator& it,
                            const std::string::iterator& end)
    {
        while (it != end && (*it == ' ' || *it == '\t'))
            ++it;
    }

    void consume_backwards_whitespace(std::string::iterator& back,
                                      const std::string::iterator& front)
    {
        while (back != front && (*back == ' ' || *back == '\t'))
            --back;
    }

    void eol_or_comment(const std::string::iterator& it,
                        const std::string::iterator& end)
    {
        if (it != end && *it != '#')
            throw_parse_exception("Unidentified trailing character "
                                  + std::string{*it}
                                  + "---did you forget a '#'?");
    }

    bool is_number(char c)
    {
        return c >= '0' && c <= '9';
    }

    bool is_date(const std::string::iterator& it,
                 const std::string::iterator& end)
    {
        auto date_end = find_end_of_date(it, end);
        std::string to_match{it, date_end};
#if CPPTOML_HAS_STD_REGEX
        return std::regex_match(to_match, date_pattern_);
#else
        // this can be approximate; we just need a lookahead
        return to_match.length() >= 20 && to_match[4] == '-'
               && to_match[7] == '-' && to_match[10] == 'T'
               && to_match[13] == ':' && to_match[16] == ':';
#endif
    }

    std::istream& input_;
    std::string line_;
    std::size_t line_number_ = 0;
#if CPPTOML_HAS_STD_REGEX
    std::regex date_pattern_{
        "(\\d{4})-(\\d{2})-(\\d{2})T(\\d{2}):(\\d{2}):(\\d{"
        "2})(\\.\\d+)?(Z|(\\+|-)(\\d{2}):(\\d{2}))"};
#endif
};

/**
 * Utility function to parse a file as a TOML file. Returns the root table.
 * Throws a parse_exception if the file cannot be opened.
 */
inline table parse_file(const std::string& filename)
{
    std::ifstream file{filename};
    if (!file.is_open())
        throw parse_exception{filename + " could not be opened for parsing"};
    parser p{file};
    return p.parse();
}
}
#endif

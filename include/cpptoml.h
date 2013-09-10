/**
 * @file cpptoml.h
 * @author Chase Geigle
 * @date May 2013
 */

#ifndef _CPPTOML_H_
#define _CPPTOML_H_

#include <algorithm>
#include <cstdint>
#include <ctime>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <memory>
#include <regex>
#include <string>
#include <unordered_map>
#include <unordered_set>

namespace cpptoml {

template <class T>
class toml_value;

/**
 * A generic base TOML value used for type erasure.
 */
class toml_base : public std::enable_shared_from_this<toml_base> {
    public:
        /**
         * Determines if the given TOML element is a value.
         */
        virtual bool is_value() const {
            return false;
        }

        /**
         * Determines if the given TOML element is a group.
         */
        virtual bool is_group() const {
            return false;
        }

        /**
         * Prints the TOML element to the given stream.
         */
        virtual void print( std::ostream & stream ) const = 0;

        /**
         * Attempts to coerce the TOML element into a concrete TOML value
         * of type T.
         */
        template <class T>
        std::shared_ptr<toml_value<T>> as();
};

/**
 * A concrete TOML value representing the "leaves" of the "tree".
 */
template <class T>
class toml_value : public toml_base {
    public:
        /**
         * Constructs a value from the given data.
         */
        toml_value( const T & val ) : data_{ val } { }

        bool is_value() const override {
            return true;
        }

        /**
         * Gets the data associated with this value.
         */
        T & value() {
            return data_;
        }

        /**
         * Gets the data associated with this value. Const version.
         */
        const T & value() const {
            return data_;
        }

        void print( std::ostream & stream ) const override {
            stream << data_;
        }

    private:
        T data_;
};

// I don't think I'll ever comprehend why this is needed...
template <>
inline toml_value<std::tm>::toml_value( const std::tm & date ) {
    data_ = date;
}

// specializations for printing nicely
template <>
inline void toml_value<bool>::print( std::ostream & stream ) const {
    if( data_ )
        stream << "true";
    else
        stream << "false";
}

template <>
inline void toml_value<std::tm>::print( std::ostream & stream ) const {
    stream << std::put_time( &data_, "%c %Z" );
}

template <>
inline void toml_value<std::vector<std::shared_ptr<toml_base>>>::print( std::ostream & stream ) const {
    stream << "[ ";
    auto it = data_.begin();
    while( it != data_.end() ) {
        (*it)->print( stream );
        if( ++it != data_.end() )
            stream << ", ";
    }
    stream << " ]";
}

template <class T>
inline std::shared_ptr<toml_value<T>> toml_base::as() {
    if( auto v = std::dynamic_pointer_cast<toml_value<T>>( shared_from_this() ) )
        return v;
    else
        return nullptr;
}

/**
 * Represents a TOML keygroup.
 */
class toml_group : public toml_base {
    public:
        /**
         * toml_groups can be iterated over.
         */
        using iterator = std::unordered_map<std::string, std::shared_ptr<toml_base>>::iterator;

        /**
         * toml_groups can be iterated over. Const version.
         */
        using const_iterator = std::unordered_map<std::string, std::shared_ptr<toml_base>>::const_iterator;

        iterator begin() {
            return map_.begin();
        }

        const_iterator begin() const {
            return map_.begin();
        }

        iterator end() {
            return map_.end();
        }

        const_iterator end() const {
            return map_.end();
        }

        bool is_group() const override {
            return true;
        }

        /**
         * Determines if ths key group contains the given key.
         */
        bool contains( const std::string & key ) const {
            return map_.find( key ) != map_.end();
        }

        /**
         * Obtains the toml_base for a given key.
         * @throw std::out_of_range if the key does not exist
         */
        std::shared_ptr<toml_base> get( const std::string & key ) const {
            return map_.at( key );
        }

        /**
         * Obtains a toml_group for a given key, if possible.
         */
        std::shared_ptr<toml_group> get_group( const std::string & key ) {
            return std::dynamic_pointer_cast<toml_group>( get( key ) );
        }

        /**
         * Helper function that attempts to get a toml_value corresponding
         * to the teplate parameter from a given key.
         */
        template <class T>
        T * get_as( const std::string & key ) {
            if( !contains( key ) )
                return nullptr;
            if( auto v = get( key )->as<T>() )
                return &v->value();
            return nullptr;
        }

        /**
         * Adds an element to the keygroup.
         */
        void insert( const std::string & key, const std::shared_ptr<toml_base> & value ) {
            map_[ key ] = value;
        }

        /**
         * Convenience shorthand for adding a simple element to the
         * keygroup.
         */
        template <class T>
        void insert( const std::string & key, T && value,
                     typename std::enable_if<std::is_same<T, std::string>::value
                                             || std::is_same<T, int64_t>::value
                                             || std::is_same<T, double>::value
                                             || std::is_same<T, bool>::value
                                             || std::is_same<T, std::tm>::value
                                            >::type * = 0 ) {
            map_[ key ] = std::make_shared<toml_value<T>>( value );
        }

        friend std::ostream & operator<<( std::ostream & stream, const toml_group & group );

        void print( std::ostream & stream ) const override {
            print( stream, 0 );
        }

    private:
        void print( std::ostream & stream, size_t depth ) const {
            for( auto & p : map_ ) {
                stream << std::string( depth, '\t' )
                    << p.first << " = ";
                if( auto g = std::dynamic_pointer_cast<toml_group>( p.second ) ) {
                    stream << '\n';
                    g->print( stream, depth + 1 );
                } else {
                    p.second->print( stream );
                    stream << '\n';
                }
            }
        }
        std::unordered_map<std::string, std::shared_ptr<toml_base>> map_;
};

/**
 * Convenience function to avoid having to type "template" when getting a
 * value out of a keygroup.
 */
template <class T>
T * get_as( const cpptoml::toml_group & group, const std::string & key ) {
        return group.get_as<T>( key );
}

inline std::ostream & operator<<( std::ostream & stream, const toml_group & group ) {
    group.print( stream );
    return stream;
}

/**
 * Exception class for all TOML parsing errors.
 */
class toml_parse_exception : public std::runtime_error {
    public:
        toml_parse_exception( const std::string & err ) : std::runtime_error{ err } { }
};

/**
 * The parser class.
 */
class parser {
    public:
        /**
         * Parsers are constructed from streams.
         */
        parser( std::istream & stream ) : input_{ stream } { }

        /**
         * Parses the stream this parser was created on until EOF.
         * @throw toml_parse_exception if there are errors in parsing
         */
        toml_group parse() {
            toml_group root;

            toml_group * curr_group = &root;

            while( std::getline( input_, line_ ) ) {
                auto it = line_.begin();
                auto end = line_.end();
                consume_whitespace( it, end );
                if( it == end || *it == '#' )
                    continue;
                if( *it == '[' ) {
                    curr_group = &root;
                    parse_group( it, end, curr_group );
                } else {
                    parse_key_value( it, end, curr_group );
                }
            }
            groups_.clear();
            return root;
        }

    private:

        void parse_group( std::string::iterator & it,
                          const std::string::iterator & end,
                          toml_group * & curr_group ) {
            // remove the beginning and ending keygroup markers
            ++it;
            auto ob = std::find( it, end, '[' );
            if( ob != end )
                throw toml_parse_exception{ "Cannot have [ in keygroup name" };
            auto kg_end = std::find( it, end, ']' );
            if( it == kg_end )
                throw toml_parse_exception{ "Empty keygroup" };
            std::string group{ it, kg_end };
            if( groups_.find( group ) != groups_.end() )
                throw toml_parse_exception{ "Duplicate keygroup" };
            groups_.insert( { it, kg_end } );
            while( it != kg_end ) {
                auto dot = std::find( it, kg_end, '.' );
                // get the key part
                std::string part{ it, dot };
                if( part.empty() )
                    throw toml_parse_exception{ "Empty keygroup part" };
                it = dot;
                if( it != kg_end )
                    ++it;

                if( curr_group->contains( part ) ) {
                    auto b = curr_group->get( part );
                    if( !b->is_group() )
                        throw toml_parse_exception{ "Keygroup already exists as a value" };
                    curr_group = static_cast<toml_group *>( b.get() );
                } else {
                    curr_group->insert( part, std::make_shared<toml_group>() );
                    curr_group = static_cast<toml_group *>( curr_group->get( part ).get() );
                }
            }
            ++it;
            consume_whitespace( it, end );
            eol_or_comment( it, end );
        }

        void parse_key_value( std::string::iterator & it,
                              std::string::iterator & end,
                              toml_group * & curr_group ) {
            std::string key = parse_key( it, end );
            if( curr_group->contains( key ) )
                throw toml_parse_exception{ "Key " + key + " already present" };
            if( *it != '=' )
                throw toml_parse_exception{ "KeyValue must contain a '='" };
            ++it;
            consume_whitespace( it, end );
            curr_group->insert( key, parse_value( it, end ) );
            consume_whitespace( it, end );
            eol_or_comment( it, end );
        }

        std::string parse_key( std::string::iterator & it,
                               const std::string::iterator & end ) {
            consume_whitespace( it, end );
            auto eq = std::find( it, end, '=' );
            auto key_end = eq;
            --key_end;
            consume_backwards_whitespace( key_end, it );
            ++key_end;
            std::string key{ it, key_end };
            it = eq;
            consume_whitespace( it, end );
            return key;
        }

        enum class parse_type {
            STRING = 1, DATE, INT, FLOAT, BOOL, ARRAY
        };

        std::shared_ptr<toml_base> parse_value( std::string::iterator & it,
                                                std::string::iterator & end ) {
            parse_type type = determine_value_type( it, end );
            switch( type ) {
                case parse_type::STRING:
                    return parse_string( it, end );
                case parse_type::DATE:
                    return parse_date( it, end );
                case parse_type::INT:
                case parse_type::FLOAT:
                    return parse_number( it, end );
                case parse_type::BOOL:
                    return parse_bool( it, end );
                case parse_type::ARRAY:
                    return parse_array( it, end );
                default:
                    throw toml_parse_exception{ "Failed to parse value" };
            }
        }

        parse_type determine_value_type( const std::string::iterator & it,
                                         const std::string::iterator & end ) {
            if( *it == '"' ) {
                return parse_type::STRING;
            } else if( is_date( it, end ) ) {
                return parse_type::DATE;
            } else if( is_number( *it ) || *it == '-' ) {
                return determine_number_type( it, end );
            } else if( *it == 't' || *it == 'f' ) {
                return parse_type::BOOL;
            } else if( *it == '[' ) {
                return parse_type::ARRAY;
            }
            throw toml_parse_exception{ "Failed to parse value type" };
        }

        parse_type determine_number_type( const std::string::iterator & it,
                                          const std::string::iterator & end ) {
            // determine if we are an integer or a float
            auto check_it = it;
            if( *check_it == '-' )
                ++check_it;
            while( check_it != end && is_number( *check_it ) )
                ++check_it;
            if( check_it != end && *check_it == '.' ) {
                ++check_it;
                while( check_it != end && is_number( *check_it ) )
                    ++check_it;
                return parse_type::FLOAT;
            } else {
                return parse_type::INT;
            }
        }

        std::shared_ptr<toml_value<std::string>> parse_string( std::string::iterator & it,
                                                               const std::string::iterator & end ) {
            ++it;
            std::string value;
            while( it != end ) {
                // handle escaped characters
                if( *it == '\\' ) {
                    value += parse_escape_code( it, end );
                } else if( *it == '"' ) {
                    ++it;
                    consume_whitespace( it, end );
                    return std::make_shared<toml_value<std::string>>( value );
                } else {
                    value += *it++;
                }
            }
            throw toml_parse_exception{ "Unended string literal" };
        }

        char parse_escape_code( std::string::iterator & it,
                                const std::string::iterator & end ) {
            ++it;
            if( it == end )
                throw toml_parse_exception{ "Invalid escape sequence" };
            char value;
            if( *it == 'b' ) {
                value = '\b';
            } else if( *it == 't' ) {
                value = '\t';
            } else if( *it == 'n' ) {
                value = '\n';
            } else if( *it == 'f' ) {
                value = '\f';
            } else if( *it == 'r' ) {
                value = '\r';
            } else if( *it == '"' ) {
                value = '"';
            } else if( *it == '/' ) {
                value = '/';
            } else if( *it == '\\' ) {
                value = '\\';
            } else {
                throw toml_parse_exception{ "Invalid escape sequence" };
            }
            ++it;
            return value;
        }

        std::shared_ptr<toml_base> parse_number( std::string::iterator & it,
                                                 const std::string::iterator & end ) {
            // determine if we are an integer or a float
            auto check_it = it;
            if( *check_it == '-' )
                ++check_it;
            while( check_it != end && is_number( *check_it ) )
                ++check_it;
            if( check_it != end && *check_it == '.' ) {
                ++check_it;
                if( check_it == end )
                    throw toml_parse_exception{ "Floats must have trailing digits" };
                while( check_it != end && is_number( *check_it ) )
                    ++check_it;
                return parse_float( it, check_it );
            } else {
                return parse_int( it, check_it );
            }
        }

        std::shared_ptr<toml_value<int64_t>> parse_int( std::string::iterator & it,
                                                  const std::string::iterator & end ) {
            std::string v{ it, end };
            it = end;
            return std::make_shared<toml_value<int64_t>>( std::stoll( v ) );
        }

        std::shared_ptr<toml_value<double>> parse_float( std::string::iterator & it,
                                                const std::string::iterator & end ) {
            std::string v{ it, end };
            it = end;
            return std::make_shared<toml_value<double>>( std::stod( v ) );
        }

        std::shared_ptr<toml_value<bool>> parse_bool( std::string::iterator & it,
                                               const std::string::iterator & end ) {
            auto boolend = std::find_if( it, end, []( char c ) {
                return c == ' ' || c == '\t' || c == '#';
            });
            std::string v{ it, boolend };
            it = boolend;
            if( v == "true" )
                return std::make_shared<toml_value<bool>>( true );
            else if( v == "false" )
                return std::make_shared<toml_value<bool>>( false );
            else
                throw toml_parse_exception{ "Attempted to parse invalid boolean value" };
        }

        std::shared_ptr<toml_value<std::tm>> parse_date( std::string::iterator & it,
                                               const std::string::iterator & end ) {
            auto date_end = std::find_if( it, end, [this]( char c ) {
                return !is_number( c ) && c != 'T' && c != 'Z' && c != ':' && c != '-';
            });
            std::string to_match{ it, date_end };
            it = date_end;
            std::regex pattern{ "(\\d{4})-(\\d{2})-(\\d{2})T(\\d{2}):(\\d{2}):(\\d{2})Z" };
            std::match_results<std::string::const_iterator> results;
            std::regex_match( to_match, results, pattern );

            // populate extracted vaues
            std::tm date;
            std::memset( &date, '\0', sizeof( date ) );
            date.tm_year = stoi( results[1] ) - 1900;
            date.tm_mon = stoi( results[2] ) - 1;
            date.tm_mday = stoi( results[3] );
            date.tm_hour = stoi( results[4] );
            date.tm_min = stoi( results[5] );
            date.tm_sec = stoi( results[6] );

            // correctly fill in missing values
            // "portable" version of timegm()
            char * tz = getenv( "TZ" );
            setenv( "TZ", "", 1 );
            tzset();
            time_t t = mktime( &date );
            date = *gmtime( &t );
            if( tz )
                setenv( "TZ", tz, 1 );
            else
                unsetenv( "TZ" );
            tzset();

            return std::make_shared<toml_value<std::tm>>( date );
        }

        std::shared_ptr<toml_base> parse_array( std::string::iterator & it,
                                                std::string::iterator & end ) {
            // this gets ugly because of the "heterogenous" restriction:
            // arrays can either be of only one type, or contain arrays
            // (each of those arrays could be of different types, though)
            //
            // because of the latter portion, we don't really have a choice
            // but to represent them as arrays of base values...
            ++it;

            // ugh---have to read the first value to determine array type...
            skip_whitespace_and_comments( it, end );

            // edge case---empty array
            if( *it == ']' ) {
                ++it;
                return std::make_shared<toml_value<std::vector<std::shared_ptr<toml_base>>>>( std::vector<std::shared_ptr<toml_base>>{} );
            }


            auto val_end = std::find_if( it, end, []( char c ) {
                return c == ',' || c == ']' || c == '#';
            });
            parse_type type = determine_value_type( it, val_end );
            switch( type ) {
                case parse_type::STRING:
                    return parse_value_array<std::string>( it, end );
                case parse_type::INT:
                    return parse_value_array<int64_t>( it, end );
                case parse_type::FLOAT:
                    return parse_value_array<double>( it, end );
                case parse_type::DATE:
                    return parse_value_array<std::tm>( it, end );
                case parse_type::ARRAY:
                    return parse_value_array<std::vector<std::shared_ptr<toml_base>>>(
                            it, end );
                default:
                    throw toml_parse_exception{ "Unable to parse array" };
            }
        }

        template <class Value>
        std::shared_ptr<toml_value<std::vector<std::shared_ptr<toml_base>>>>
        parse_value_array( std::string::iterator & it,
                           std::string::iterator & end ) {
            std::vector<std::shared_ptr<toml_base>> array;
            while( it != end && *it != ']' ) {
                auto value = parse_value( it, end );
                if( auto v = value->as<Value>() )
                    array.push_back( value );
                else
                    throw toml_parse_exception{ "Arrays must be heterogeneous" };
                skip_whitespace_and_comments( it, end );
                if( *it != ',' )
                    break;
                ++it;
                skip_whitespace_and_comments( it, end );
            }
            if( it != end )
                ++it;
            return std::make_shared<toml_value<decltype(array)>>( array );
        }

        void skip_whitespace_and_comments( std::string::iterator & start,
                                           std::string::iterator & end ) {
            consume_whitespace( start, end );
            while( start == end || *start == '#' ) {
                if( !std::getline( input_, line_ ) )
                    throw toml_parse_exception{ "Unclosed array" };
                start = line_.begin();
                end = line_.end();
                consume_whitespace( start, end );
            }
        }

        void consume_whitespace( std::string::iterator & it, const std::string::iterator & end ) {
            while( it != end && ( *it == ' ' || *it == '\t' ) )
                ++it;
        }

        void consume_backwards_whitespace( std::string::iterator & back,
                                           const std::string::iterator & front ) {
            while( back != front && (*back == ' ' || *back == '\t' ) )
                --back;
        }

        void eol_or_comment( const std::string::iterator & it,
                             const std::string::iterator & end ) {
            if( it != end && *it != '#' )
                throw toml_parse_exception{ "Unidentified trailing character " + std::string{ *it } + "---did you forget a '#'?" };
        }

        bool is_number( char c ) {
            return c >= '0' && c <= '9';
        }

        bool is_date( const std::string::iterator & it,
                      const std::string::iterator & end ) {
            auto date_end = std::find_if( it, end, [this]( char c ) {
                return !is_number( c ) && c != 'T' && c != 'Z' && c != ':' && c != '-';
            });
            std::string to_match{ it, date_end };
            std::regex pattern{ "\\d{4}-\\d{2}-\\d{2}T\\d{2}:\\d{2}:\\d{2}Z" };
            return std::regex_match( to_match, pattern );
        }

        std::istream & input_;
        std::string line_;
        std::unordered_set<std::string> groups_;
};

toml_group parse_file( const std::string & filename ) {
    std::ifstream file{ filename };
    parser p{ file };
    return p.parse();
}

}
#endif

#pragma once

#include "redis_endpoint.h"
#include "redis_reply_iterator.h"
#include <string>
#include <utility>
#include <algorithm>

namespace redis
{
   
    // this would be used to generate messages from and to the server redis
    // note that this accept either build it types such as int, short, string, or datatypes
    // that can be converted to string.
    // also note that this would allow to either set <key, value> pair, publish or subscribe on channel
    // the data that we are reading would be placed into reply object, and the status of the writing
    // is just a boolean value - whether this was successfull or not

    /*
    usage:
        end_point connection(..);
        rstring str(ep, "my string");
        str = "this is the start of the string";
        str += " and this is the end of the string";
        std::cout<<"the string is "<<str.str()<<std::endl; // would print 'this is the start of the string and this is the end of the string'
        std::cout<<"this string length is "<<str.size()<<std::endl; // would print 61
        std::cout<<"sub str for range 4 - 8 is "str.substr()<<std::endl;    // would print ' is ';
        std::cout<<"char at 2 is '"<<str[2]<<"'<<std::endl; // would print i
    */
    struct rstring
    {
        typedef std::pair<int, int> range_type;
        // initiation of a string as thourgh the connection point and the name that we 
        // would use to set values to
        rstring(end_point ep, const std::string& name);

        // this function would set new value to the string set by ctor
        rstring& operator = (const std::string& value);

        // this would append string or set new value if not exists
        rstring& operator +=(const std::string& add_str);

        // see above operator +=
        void append(const std::string& add_str);

        // get the stored value of the string - return NULL if nothing there
        std::string str() const;
    
        // return the length of the string stored
        std::string::size_type size() const;

        // return the char at a given index 
        std::string::value_type operator [] (int at) const;

        // return substring - note that negative values would start from the end.
        std::string substr (const range_type& at) const;

        std::string operator () (int from, int to) const;

        // remove this entry - note it would not be possible to use this again..
        void erase();

    private:
        std::string key_name;
        mutable end_point   connection;
    };

    // this would map between a key and a value
    // please note that we are not really holding anything there
    // this is just an interface for this type of access 
    // we cannot iterate over this and we cannot just find anything
    // we are only making this look like stl map to a point

    // note that since this can access any key in the database, getting all keys to iterate over them - 
    // as in stl map is considered bad practice. For this reason, there is no begin/end and 
    // even the functions size and empty should not be called noramally!

    /*
    usage:
    end_point connection(..);
    map map(connection);
    map["some key"] = "some value";
    map["other key"] = "other value";
    map["hello"] = "world";
    map["foo"] = "bar";
    map.insert("key", "value");
    std::cout<<"the value of key 'foo' is "<<map.find("foo")<<std::endl;    // would print bar
    map.erase("foo");
    std::cout<<"the value of key 'foo' is "<<map.find("foo")<<std::endl;    // would print nothing
    */
    struct rmap
    {
        typedef std::string                         key_type;
        typedef std::string                         string_type;    // first is payload, second is length
        typedef string_type                         mapped_type;
        typedef std::pair<key_type, mapped_type>    value_type;

        explicit rmap(end_point ep);

        // unlike std::map this would not allow for modification
        // this is the same as find
        mapped_type operator [] (const key_type& key) const;

        // insert new value based on key - return false if failed
        bool insert(const key_type& key, mapped_type value) const;

        bool insert(const value_type& new_val) const;

        // insert a range of elements - note that each of which must be 
        // type of value_type or convert to it
        template<typename It>
        void insert_range(It from, It to)
        {
            
            std::for_each(from, to, [this](const auto& v) {
                this->insert(v);
            });
        }

        // same as operator [] - return the value if found, otherwise return NULL
        mapped_type find(const key_type& key) const;

        void erase(const key_type& k) const;

        // return the size of this map
        std::size_t size() const;

        // return true if this is empty
        bool empty() const;

    private:
        mutable end_point connection;
    };

    // store and retrive iteger value
    /*
    usage:
    end_point connection(..);
    long_int my_conter(connection, "my_count", 25);
    std::cout<<"value of counter is "<<static_cast<long_int::value_type>(my_conter)<<std::endl; // would print 25
    my_counter = 40;
    std::cout<<"value of counter is "<<static_cast<long_int::value_type>(my_conter)<<std::endl; // would print 40
    ++my_counter;
    my_counter--;
    std::cout<<"value of counter is "<<my_conter()<<std::endl; // would print 40
    my_counter += 234;
    my_counter -= 234;
    std::cout<<"value of counter is "<<*my_conter<<std::endl; // would print 40
    std::cout<<"my counter is "<<(my_coutner == 40 ? "equals" : "not equals")<<" to 40"<<std::endl;
    */
    struct long_int
    {
        typedef unsigned long long value_type;

        long_int(end_point c, const std::string& name, value_type v);

        long_int(end_point c, const std::string& name);

        long_int& operator = (value_type val);  // reset the stored value
    
        operator value_type () const;   // return the stored value

        value_type operator ++ () const;    // add one to the stored value pre
    
        value_type operator ++ (int) const;// add one to the stored value post

        value_type operator -- () const;    // remove one fromt the stored value pre
    
        value_type operator -- (int) const; // remove one from the stored value post
            
        value_type operator += (value_type by) const;   // add "by" to the stored value
    
        value_type operator -= (value_type by) const;   // remove  "by" to the stored value

        value_type operator *() const;                  // return the stored value

        value_type operator () () const;                   // return the stored value
    
    private:
        mutable end_point connection;
        std::string name;
    };
    // copare this to other numbers or other long_int
    bool operator == (const long_int& ulr, const long_int& ull);
    bool operator != (const long_int& ulr, const long_int& ull);
    bool operator < (const long_int& ulr, const long_int& ull);
    bool operator > (const long_int& ulr, const long_int& ull);
    bool operator >= (const long_int& ulr, const long_int& ull);
    bool operator <= (const long_int& ulr, const long_int& ull);

    bool operator == (const long_int::value_type ulr, const long_int& ull);
    bool operator != (const long_int::value_type ulr, const long_int& ull);
    bool operator < (const long_int::value_type ulr, const long_int& ull);
    bool operator > (const long_int::value_type ulr, const long_int& ull);
    bool operator >= (const long_int::value_type ulr, const long_int& ull);
    bool operator <= (const long_int::value_type ulr, const long_int& ull);

    bool operator == (const long_int& ulr, const long_int::value_type ull);
    bool operator != (const long_int& ulr, const long_int::value_type ull);
    bool operator < (const long_int& ulr, const long_int::value_type ull);
    bool operator > (const long_int& ulr, const long_int::value_type ull);
    bool operator >= (const long_int& ulr, const long_int::value_type ull);
    bool operator <= (const long_int& ulr, const long_int::value_type ull);

    // this would have the same iteraface as array - 
    // note that in this case we assume to have no mapping
    // of value to a given key - but rather a list of values
    // assusiated with a key or without any key at all

    
    /*usage:
    end_point connection(..);
    rarray my_array(connection, "my array");
    my_array.push_back("at the back");
    my_array.push_front("at the front");
    std::cout<<"my array is "<<(my_array.empty() ? "empty" : "not empty")<<" and have "<<my_array.size()<<" elements\n";    // would print no empty and 2
    std::copy(my_array.begin(), my_array.end(), std::ostream_iterator<reply>(std::cout, ", ")); // print 'at the front', 'at the back'
    my_array.erase();
    std::cout<<"my array is "<<(my_array.empty() ? "empty" : "not empty")<<" and have "<<my_array.size()<<" elements\n"; // would print empty and 0
    */
    struct rarray
    {
        typedef rmap::string_type   result_type;
        typedef rmap::string_type   string_type;
        typedef reply_iterator      iterator;
        typedef std::size_t         size_type;

        // note that arrays on redis must initiate with conenction and the array identifier
        rarray(end_point c, const std::string& name);

        // add new entry to the array
        bool push_back(const string_type& value);

        // add new entry ot the array at the front
        bool push_front(const string_type& value);

        // get entry from the array basesd on location (index)
        result_type operator [] (size_type at) const; 

        // same as operator []
        result_type at(size_type at) const; 

        // return iterator to the first element of the array - if empty return end()
        iterator begin() const;

        // return iterator to the element one pass the last element in the array - cannot be dereference
        iterator end() const;

        // return the number of entries in the array
        size_type size() const;

        // return true if array is empty
        bool empty() const;

        // remore this array (same as clear in stl)
        void erase();

    private:
        mutable end_point connection;
        std::string name;
    };
}   // end of namespace redis


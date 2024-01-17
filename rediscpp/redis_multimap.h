#pragma once

#include "redis_endpoint.h"
#include "redis_reply.h"
#include "redis_reply_iterator.h"
#include <string>
#include <utility>
#include <boost/iterator/iterator_facade.hpp>
#include <optional>

namespace redis
{

/* usage:
  end_point connection(..);
  rmultimap my_multimap(connection);
  my_multimap["foo"]["bar"] = "this is the value of bar";
  my_multimap["foo"]["hello"] = "world";
  my_multimap["foo"]["one"] = "1";
  my_multimap["foo"]["two"] = "2";
  if (!my_multimap["foo"].empty()) {
      std::cout<<"no emptry\n";
  }
  std::cout<<"the number of entries under foo is "<<my_multimap["foo"].size()<<std::endl;
  // print all keys
  std::copy(my_multimap["foo"].keys_begin(), my_multimap["foo"].keys_end(), std::ostream_iterator<std::string>(std::cout, ", "));
  // print all entries
  std::ostream& operator <<(std::ostream& os, const multimap_iterator::result_type&);

  std::copy(my_multimap["foo"].begin(), my_multimap["foo"].end(), std::ostream_iterator<multimap_iterator::result_type>(std::cout, ", "));
  // you can use other stl algorithms and functions but I would not show it here
  // now we would remove all entries from the map
  my_multimap.clear();
  assert(my_multimap["foo"].empty());
*/

class rmultimap;


// allow iteration over elements in the multimap (see below)
struct multimap_iterator : public boost::iterator_facade<multimap_iterator, // this type
                                                         const std::pair<std::string, std::string>, // result reference
                                                         boost::random_access_traversal_tag,        // type of trasverse
                                                         std::pair<std::string, std::string>        // result type
                           >
{
    typedef std::string                     key_type;
    typedef std::string                     mapped_type;
    typedef std::pair<key_type, mapped_type> result_type;

    multimap_iterator(result::array&& r); 

    multimap_iterator();    // for end iterator

private:
    friend class boost::iterator_core_access;
    void increment();

    void decrement();

    void advance(difference_type n);   // move forward n steps note - must be even number!!

    bool equal(multimap_iterator const& other) const;

    result_type dereference() const;
private:
    reply_iterator current;
};

// allow iterating over all the keys 
struct multimap_key_iterator : public boost::iterator_facade<multimap_key_iterator , 
                                                             const multimap_iterator::key_type, 
                                                              boost::random_access_traversal_tag,        // type of trasverse
                                                              const multimap_iterator::key_type
                               >
{
    multimap_key_iterator();    // for end();
    multimap_key_iterator(result::array&& r);

private:
    void increment();

    void decrement();

    void advance(difference_type n);   // move forward n steps note - must be even number!!

    bool equal(multimap_key_iterator const& other) const;

    const multimap_iterator::key_type  dereference() const;

private:
    reply_iterator current;
};
// This class is used as an enty in the rmultimap class bellow
// this class is a collection of <key, value> pairs and the user
// can iterate over it and search it, the user and also 
// check for this list size and whether this is empty
class rmmap_proxy
{
public:
    typedef multimap_iterator::key_type key_type;
    typedef multimap_iterator::mapped_type mapped_type;    // we would only store strings, if the object is not a string then turn it to string with boost::lexical_cast
    typedef multimap_iterator::result_type value_type;
    // note that for the iterator - the odd place iterators would be the key name,
    // and the even indecis are values
    typedef multimap_iterator    iterator; 
    typedef multimap_iterator    const_iterator;
    typedef multimap_key_iterator keys_iterator;
    typedef multimap_key_iterator const_keys_iterator;

private:
    rmmap_proxy(const std::string& pk, end_point* e);
    
    friend class rmultimap;
public:
    std::optional<mapped_type> operator [] (const key_type& key) const; // unlike stl you are not allow to change the value inside a map

    bool insert(const value_type& new_entry) const; // new entry into redis
    
    bool insert(const key_type& key, const mapped_type& value) const;   // insert key value pair into redis

    bool empty() const;                         // return true if no entry for this primary key

    std::size_t size() const;                   // return the number of elements under the same primary key

    iterator begin();                           // return iterator to the beginning of the <key, value> list of entries

    iterator end();                             // return iterator to the end of the list of entries to the primary key

    const_iterator begin() const;               // return iterator to the beginning of the <key, value> list of entries

    const_iterator end() const;                 // return iterator to the end of the list of entries to the primary key

    keys_iterator keys_begin();                 /// iterator to the start of list of all keys

    keys_iterator keys_end();                   // iterator to the end of list of all keys (cannot be deference)

    const_keys_iterator keys_begin() const;     /// iterator to the start of list of all keys

    const_keys_iterator keys_end() const;       // iterator to the end of list of all keys (cannot be deference)

    std::optional<std::string> find(const key_type& key) const;  // find entry in the primary key entry with a given key

    void erase(const key_type& key);            // remove entry from the primary key


private:
    std::string pkey;
    end_point* ep;
};

// This class is in a sense the same as STL multi map,
// we have a primary key that is used for lookup and then 
// unlike the multimap of STL we can fetch an internal 
// object using a "tag" or a secondary key into the storage
class rmultimap
{
public:
    typedef rmmap_proxy::key_type key_type;
    typedef rmmap_proxy mapped_type;    // we would only store strings, if the object is not a string then turn it to string with boost::lexical_cast
    typedef rmmap_proxy::value_type value_type;

    explicit rmultimap(end_point ep);                   // to start using this we must have a connection to the server

    rmmap_proxy operator [] (const key_type& at);       // not allowing to change the value directly here

    void insert(const key_type&, const value_type&);    // this would generate a new key if not found and insert a value to the key

    void clear(const key_type& key) ;                   // remove any entries associated with a given key that is control by this

private:
    end_point endpoint;
};

}   // namespace redis


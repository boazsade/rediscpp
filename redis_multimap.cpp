#include "redis_multimap.h"
#include <hiredis/hiredis.h>
#include <iterator>

namespace redis
{

multimap_iterator::multimap_iterator(const reply* r) : current(r)
{
}

multimap_iterator::multimap_iterator() : current((const reply*)0)
{
}

void multimap_iterator::increment()
{
    // moving forward is alway by two steps!
   ++current;
   ++current;
}

void multimap_iterator::decrement()
{
    // moving back is always 2 steps
    --current;
    --current;
}

void multimap_iterator::advance(difference_type step)
{
    step *= 2;  // we are moving 2 positions at a time
    std::advance(current, step);
}

bool multimap_iterator::equal(multimap_iterator const& other) const
{
    return other.current == current;
}

multimap_iterator::result_type multimap_iterator::dereference() const
{
    static const reply_iterator dummy = reply_iterator((const reply*)0);

    if (current != dummy && at(current) % 2 == 0) { // make sure that we can dereference from legal value!
        reply_iterator tmp(current);
        key_type k(current->answer(), current->size());
        ++tmp;
        mapped_type m(tmp->answer(), tmp->size());
        return result_type(k, m);
    } else {
        static const result_type error = result_type(key_type(), mapped_type());
        return error;
    }
}

///////////////////////////////////////////////////////////////////////////////

multimap_key_iterator::multimap_key_iterator() : current((const reply*)0)
{
}

multimap_key_iterator::multimap_key_iterator(const reply* r) : current(r)
{
}

void multimap_key_iterator::increment()
{
   ++current;
}

void multimap_key_iterator::decrement()
{
    --current;
}

void multimap_key_iterator::advance(difference_type step)
{
    std::advance(current, step);
}

bool multimap_key_iterator::equal(multimap_key_iterator const& other) const
{
    return other.current == current;
}

const multimap_iterator::key_type multimap_key_iterator::dereference() const
{
    static const reply_iterator dummy = reply_iterator((const reply*)0);

    if (current != dummy) {
        multimap_iterator::key_type k(current->answer(), current->size());
        return k;
    } else {
        static const multimap_iterator::key_type error = multimap_iterator::key_type();
        return error;
    }
}


///////////////////////////////////////////////////////////////////////////////

rmultimap::rmultimap(end_point ep) :endpoint(ep)
{
}

rmmap_proxy rmultimap::operator [] (const key_type& at) 
{
    return rmmap_proxy(at, &endpoint);
}

void rmultimap::insert(const key_type& key, const value_type& val)
{
    redisCommand(cast(endpoint), "HSET %b %b %b", key.data(), key.size(),
            val.first.data(), val.first.size(), val.second.data(), val.second.size());
}

void rmultimap::clear(const key_type& key) 
{
    redisCommand(cast(endpoint), "DEL %b", key.data(), key.size());
}

///////////////////////////////////////////////////////////////////////////////

rmmap_proxy::rmmap_proxy(const std::string& pk, end_point* e) : pkey(pk), ep(e)
{
}

rmmap_proxy::mapped_type rmmap_proxy::operator [] (const key_type& key) const 
{   // try to read from redis the entry with the given name
    reply r((redisReply*)redisCommand(cast(*ep), "HGET %b %b", pkey.data(), pkey.size(), key.data(), key.size()));
    if (r && r.answer()) {
        return mapped_type(r.answer(), r.size());
    } else {
        static const mapped_type error = mapped_type();
        return error;
    }
}

bool rmmap_proxy::insert(const value_type& new_entry) const
{
    reply r((redisReply*)redisCommand(cast(*ep), "HMSET %b %b %b", pkey.data(), pkey.size(), 
                new_entry.first.data(), new_entry.first.size(), new_entry.second.data(), new_entry.second.size()));
    return r.valid();
}

bool rmmap_proxy::insert(const key_type& key, const mapped_type& value) const
{
    return insert(value_type(key, value));
}

bool rmmap_proxy::empty() const
{
    return size() == 0;
}

std::size_t rmmap_proxy::size() const
{
    reply r((redisReply*)redisCommand(cast(*ep), "HLEN %b", pkey.data(), pkey.size()));

    if (r) {
        return r.get();
    } else {
        return 0;
    }
}

rmmap_proxy::keys_iterator rmmap_proxy::keys_begin()
{
    reply r((redisReply*)redisCommand(cast(*ep), "HKEYS %b", pkey.data(), pkey.size()));
    if (r) {
        return keys_iterator(&r);
    } else {
        return keys_end();
    }
}

rmmap_proxy::const_keys_iterator rmmap_proxy::keys_begin() const
{
    reply r((redisReply*)redisCommand(cast(*ep), "HKEYS %b", pkey.data(), pkey.size()));
    if (r) {
        return const_keys_iterator(&r);
    } else {
        return keys_end();
    }
}

rmmap_proxy::keys_iterator rmmap_proxy::keys_end() 
{
    static const keys_iterator end_i =  keys_iterator();
    return end_i;
}

rmmap_proxy::const_keys_iterator rmmap_proxy::keys_end() const
{
    static const const_keys_iterator end_i =  const_keys_iterator();
    return end_i;
}

rmmap_proxy::iterator rmmap_proxy::begin() 
{
    reply r((redisReply*)redisCommand(cast(*ep), "HGETALL %b", pkey.data(), pkey.size()));
    if (r) {        
        return iterator(&r);
    } else {
        return end();
    }
}

rmmap_proxy::iterator rmmap_proxy::end() 
{
    static const const_iterator invalid = const_iterator();
    return invalid;
}

rmmap_proxy::const_iterator rmmap_proxy::begin() const
{
    reply r((redisReply*)redisCommand(cast(*ep), "HGETALL %b", pkey.data(), pkey.size()));
    if (r) {
        return const_iterator(&r);
    } else {
        return end();
    }
}

rmmap_proxy::const_iterator rmmap_proxy::end() const
{
    static const const_iterator invalid = const_iterator();
    return invalid;
}

rmmap_proxy::iterator rmmap_proxy::find(const key_type& key)
{
    reply r((redisReply*)redisCommand(cast(*ep), "HGET %d %d", pkey.data(), pkey.size(), key.data(), key.size()));
    if (r) {
        return iterator(&r);
    } else {
        return end();
    }
}

void rmmap_proxy::erase(const key_type& key)
{
    redisCommand(cast(*ep), "HDEL %b %b", pkey.data(), pkey.size(), key.data(), key.size());
}


}   // end of namespace redis


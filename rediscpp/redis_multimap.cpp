#include "redis_multimap.h"
#include "rediscpp/internal/commands.h"
#include <hiredis/hiredis.h>
#include <iterator>
#include <boost/algorithm/string.hpp>

namespace redis
{

multimap_iterator::multimap_iterator(result::array&& r) : current{std::move(r)}
{
}

multimap_iterator::multimap_iterator() : current{}
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
    auto op = step > 0 ? &multimap_iterator::increment : &multimap_iterator::decrement;
    if (step < 0) {
        step *= -1;
    }
    while (step-- > 0) {
        ((*this).*op)();
    }
}

bool multimap_iterator::equal(multimap_iterator const& other) const
{
    return other.current == current;
}

multimap_iterator::result_type multimap_iterator::dereference() const
{
    using namespace std::string_literals;

    static const auto end = reply_iterator{};
    if (current != end && at(current) % 2 == 0) { // make sure that we can dereference from legal value!
        reply_iterator tmp(current);
        const auto s = result::try_into<result::string>(*current).and_then([&tmp](auto&& s) -> ::result<result_type, std::string> {
           ++tmp;
            //mapped_type m(tmp->answer(), tmp->size());
            const auto m = result::try_into<result::string>(*tmp);
            if (m.is_ok()) {
                return ok(result_type(result::to_string(s), result::to_string(m.unwrap())));
            }
            return failed("failed to convert mapped type"s);
        });
        if (s.is_ok()) {
            return s.unwrap();
        }
    }
    return {};
}

///////////////////////////////////////////////////////////////////////////////

multimap_key_iterator::multimap_key_iterator() : current{}
{
}

multimap_key_iterator::multimap_key_iterator(result::array&& r) : current{std::move(r)}
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
    auto op = step > 0 ? &multimap_key_iterator::increment : &multimap_key_iterator::decrement;
    if (step < 0) {
        step *= -1;
    }
    while (step-- > 0) {
        ((*this).*op)();
    }
}

bool multimap_key_iterator::equal(multimap_key_iterator const& other) const
{
    return other.current == current;
}

const multimap_iterator::key_type multimap_key_iterator::dereference() const
{
    static const auto end = reply_iterator{};
    if (current != end) {
        const auto r = result::try_into<result::string>(*current);
        if (r.is_ok()) {
            return result::to_string(r.unwrap());
        }
    }
    return {};
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

std::optional<rmmap_proxy::mapped_type> rmmap_proxy::operator [] (const key_type& key) const 
{   // try to read from redis the entry with the given name
    // reply r((redisReply*)redisCommand(cast(*ep), "HGET %b %b", pkey.data(), pkey.size(), key.data(), key.size()));
    // if (r && r.answer()) {
    //     return mapped_type(r.answer(), r.size());
    // } else {
    //     static const mapped_type error = mapped_type();
    //     return error;
    // }
    return find(key);
}

bool rmmap_proxy::insert(const value_type& new_entry) const
{
    const auto r = internal::process<result::status>::run(*ep, "HMSET %b %b %b", pkey.data(), pkey.size(),
                                      new_entry.first.data(), new_entry.first.size(), new_entry.second.data(), new_entry.second.size());
    return  r.is_error() ? false : boost::algorithm::iequals(r.unwrap().message(), "ok");
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
    const auto r = internal::process<result::integer>::run(
        *ep, "HLEN %b", pkey.data(), pkey.size()
    );
    
    if (r.is_ok()) {
        return r.unwrap().message();
    }
    return 0;
    
}

rmmap_proxy::keys_iterator rmmap_proxy::keys_begin()
{
    const auto r = internal::process<result::array>::run(
        *ep, "HKEYS %b", pkey.data(), pkey.size()
    );
    
    if (r.is_ok()) {
        auto a = r.unwrap();
        return keys_iterator(std::move(a));
    } else {
        return keys_end();
    }
}

rmmap_proxy::const_keys_iterator rmmap_proxy::keys_begin() const
{
    const auto r = internal::process<result::array>::run(
        *ep, "HKEYS %b", pkey.data(), pkey.size()
    );
    // auto  r = result::try_into<result::array>(result::any::from(
    //     (redisReply*)redisCommand(cast(*ep), "HKEYS %b", pkey.data(), pkey.size()))
    // );
    if (r.is_ok()) {
        auto a = r.unwrap();
        return keys_iterator(std::move(a));
    } else {
        return keys_end();
    }
}

rmmap_proxy::keys_iterator rmmap_proxy::keys_end() 
{
    return {};
}

rmmap_proxy::const_keys_iterator rmmap_proxy::keys_end() const
{
    static const const_keys_iterator end_i =  const_keys_iterator();
    return end_i;
}

rmmap_proxy::iterator rmmap_proxy::begin() {
   
    const auto arr = internal::process<result::array>::run(
        *ep, "HGETALL %b", pkey.data(), pkey.size()
    );
    if (arr.is_ok()) {
        auto a = arr.unwrap();
        return iterator(std::move(a));
    } else {
        return end();
    }
}

rmmap_proxy::iterator rmmap_proxy::end() 
{
   return {};
}

rmmap_proxy::const_iterator rmmap_proxy::begin() const
{
    const auto arr = internal::process<result::array>::run(
        *ep, "HGETALL %b", pkey.data(), pkey.size()
    );
    
    if (arr.is_ok()) {
        auto a = arr.unwrap();
        return const_iterator(std::move(a));
    } else {
        return end();
    }
}

rmmap_proxy::const_iterator rmmap_proxy::end() const
{
    return {};
}

std::optional<std::string> rmmap_proxy::find(const key_type& key) const
{
    const auto r = internal::process<result::string>::run(
        *ep, "HGET %d %d", pkey.data(), pkey.size(), key.data(), key.size()
    );
    
    if (r.is_ok()) {
        return result::to_string(r.unwrap());
    } else {
        return {};
    }
}

void rmmap_proxy::erase(const key_type& key)
{
    redisCommand(cast(*ep), "HDEL %b %b", pkey.data(), pkey.size(), key.data(), key.size());
}


}   // end of namespace redis


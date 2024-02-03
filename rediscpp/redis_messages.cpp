#include "redis_messages.h"
#include "result/results.h"
#include "redis_reply.h"
#include "result/results.h"
#include "rediscpp/internal/commands.h"
#include <hiredis/hiredis.h>
#include <boost/algorithm/string.hpp>
#include <iostream>


namespace redis
{
    namespace 
    {
        
    }   // end of local namespace

    rstring::rstring(end_point ep, const std::string& name) : key_name(name), connection(ep)
    {
    }

    rstring& rstring::operator = (const std::string& value)
    {       
        internal::process_validate<void>::run(connection,  "SET %s %b", key_name.c_str(), value.data(), value.size());
        return *this;
    }

    rstring& rstring::operator +=(const std::string& add_str)
    {
        append(add_str);
        return *this;
    }

    void rstring::append(const std::string& add_str)
    {
        internal::process_validate<void>::run(connection,  "APPEND %s %b", key_name.c_str(), add_str.data(), add_str.size());
    }

    std::string rstring::str() const
    {
        const auto r = internal::process_validate<result::string>::run(connection, "GET %s", key_name.c_str());
        
        return result::to_string(r);        
    }
    
    std::string::size_type rstring::size() const
    {
        try {
            auto s = str();
            return s.size();
        } catch (const connection_error& e) {
            throw e;
        } catch (...) {
            return 0;
        }

    }

    std::string rstring::substr (const range_type& at) const
    {
        return this->operator()(at.first, at.second);
    }

    std::string::value_type rstring::operator [] (int at) const
    {
        std::string s = substr(range_type(at, at+1));
        if (s.empty()) {
            return '\0';    // some error value..
        } else {
            return s[0];
        }
    }

    std::string rstring::operator () (int from, int to) const
    {
        const auto r = internal::process_validate<result::string>::run(connection, "GETRANGE %s %s %s", key_name.c_str(),
            std::to_string(from).c_str(),
            std::to_string(to).c_str()
        );
        
        return result::to_string(r);
    }

    void rstring::erase()
    {
        redisCommand(cast(connection), "DEL %s", key_name.c_str());
    }

///////////////////////////////////////////////////////////////////////////////
//
    
    rmap::rmap(end_point ep) : connection(ep)
    {
    }

    rmap::mapped_type rmap::operator [] (const key_type& key) const
    {
        return find(key);
    }

    bool rmap::insert(const key_type& key, mapped_type value) const
    {
        const auto r = internal::process_validate<result::status>::run(connection, "SET %s %b", 
                     key.c_str(), value.data(), value.size());
      
        return boost::algorithm::iequals(r.message(), "ok");
    }

    bool rmap::insert(const value_type& new_val) const
    {
        return insert(new_val.first, new_val.second);
    }

    rmap::mapped_type rmap::find(const key_type& key) const
    {
        if (!connection) {
            throw connection_error("trying to use invalid endpoint object to find map entry");
        }
        return rstring(connection, key).str();
    }

    void rmap::erase(const key_type& k) const
    {
        if (!connection) {
            throw connection_error("trying to use invalid endpoint object to delete map entry");
        }

        redisCommand(cast(connection), "DEL %s", k.c_str());
    }

    std::size_t  rmap::size() const
    {
        const auto r = internal::process_validate<result::integer>::run(connection, "DBSIZE");             
        return r.message();       
    }

    bool rmap::empty() const
    {
        return size() != 0;
    }

///////////////////////////////////////////////////////////////////////////////
//
    
    long_int::long_int(end_point c, const std::string& n) : connection(c), name(n)
    {
    }

    long_int::long_int(end_point c, const std::string& n, value_type v) : connection(c), name(n) {

        const auto extract = [](const auto& result) -> std::string {
            if (result.is_error()) {
                return result.error_value();
            }
            const auto s = result.unwrap();
            const auto s2 = s.message();
            return std::string(s2.data(), s2.size());
        };

        const auto r = internal::process<result::status>::run(connection, "SET %s %s", name.c_str(), std::to_string(v).c_str());
        auto em = extract(r);
        if (!boost::algorithm::iequals(em, "ok")) {
            throw connection_error(std::string(em.data(), em.size()));
        }
    }

    long_int& long_int::operator = (value_type val)
    {
        constexpr std::string_view expected = "ok";
        const auto r = internal::process<result::status>::run(connection, "SET %s %s", name.c_str(), std::to_string(val).c_str());
        const auto em = r.is_error() ? r.error_value() : r.unwrap().message();
        if (!boost::algorithm::iequals(em, expected)) {
            throw connection_error(std::string(em.data(), em.size()));
        }
        return *this;
    }
    
    long_int::operator long_int::value_type () const
    {
        const auto r = internal::process_validate<result::integer>::run(connection, "GET %s", name.c_str());        
        
        return r.message();
    }

    long_int::value_type long_int::operator ++ () const
    {
        const auto r = internal::process_validate<result::integer>::run(connection, "INCR %s", name.c_str());
              
        return r.message();
    }
    
    long_int::value_type long_int::operator ++ (int) const
    {
        auto i = this->operator()();
        
        redisCommand(cast(connection), "INCR %s", name.c_str());
        return i;
    }

    long_int::value_type long_int::operator -- () const
    {  
        const auto r = internal::process_validate<result::integer>::run(connection, "DECR %s", name.c_str());
             
        return r.message();
    }
    
    long_int::value_type long_int::operator -- (int) const
    {
        auto i = this->operator()();
        
        redisCommand(cast(connection), "DECR %s", name.c_str());
        return i;
    }
        
    long_int::value_type long_int::operator += (value_type by) const
    {
        const auto r = internal::process_validate<result::integer>::run(connection, "INCRBY %s %s", name.c_str(), std::to_string(by).c_str());
             
        return r.message();        
    }
    
    long_int::value_type long_int::operator -= (value_type by) const
    {
        const auto r = internal::process_validate<result::integer>::run(connection, "DECRBY %s %s", name.c_str(), std::to_string(by).c_str());
                
        return r.message();        
    }

    bool operator == (const long_int& ulr, const long_int& ull)
    {
        return static_cast<long_int::value_type>(ulr) == static_cast<long_int::value_type>(ull);
    }

    bool operator != (const long_int& ulr, const long_int& ull)
    {
        return !(ulr == ull);
    }

    bool operator < (const long_int& ulr, const long_int& ull)
    {
        return static_cast<long_int::value_type>(ulr) < static_cast<long_int::value_type>(ull);
    }

    bool operator > (const long_int& ulr, const long_int& ull)
    {
        return !((ulr == ull) || (ulr < ull));
    }

    bool operator >= (const long_int& ulr, const long_int& ull)
    {
        return (ulr == ull) || (ulr > ull);
    }

    bool operator <= (const long_int& ulr, const long_int& ull)
    {
        return (ulr == ull) || (ulr < ull);
    }

    bool operator == (const long_int::value_type ulr, const long_int& ull)
    {
        return ulr == static_cast<long_int::value_type>(ull);
    }

    bool operator != (const long_int::value_type ulr, const long_int& ull)
    {
        return !(ulr == ull);
    }

    bool operator < (const long_int::value_type ulr, const long_int& ull)
    {
        return ulr < static_cast<long_int::value_type>(ull);
    }

    bool operator > (const long_int::value_type ulr, const long_int& ull)
    {
        return ulr > static_cast<long_int::value_type>(ull);
    }

    bool operator >= (const long_int::value_type ulr, const long_int& ull)
    {
        return ulr == ull || ulr > ull;
    }

    bool operator <= (const long_int::value_type ulr, const long_int& ull)
    {
        return ulr < ull || ulr == ull;
    }

    bool operator == (const long_int& ulr, const long_int::value_type ull)
    {
        return static_cast<long_int::value_type>(ulr) == ull;
    }

    bool operator != (const long_int& ulr, const long_int::value_type ull)
    {
        return !(ulr == ull);
    }

    bool operator < (const long_int& ulr, const long_int::value_type ull)
    {
        return static_cast<long_int::value_type>(ulr) < ull;
    }

    bool operator > (const long_int& ulr, const long_int::value_type ull)
    {
        return static_cast<long_int::value_type>(ulr) > ull;
    }

    bool operator >= (const long_int& ulr, const long_int::value_type ull)
    {
        return ulr == ull || ulr > ull;
    }

    bool operator <= (const long_int& ulr, const long_int::value_type ull)
    {
        return ulr < ull || ulr == ull;
    }

    long_int::value_type long_int::operator *() const
    {
        return static_cast<value_type>(*this);
    }

    long_int::value_type long_int::operator () () const
    {
        return static_cast<value_type>(*this);
    }

    ///////////////////////////////////////////////////////////////////////////
    //

    rarray::rarray(end_point c, const std::string& n) : connection(c), name(n)
    {
    }

    bool rarray::push_back(const string_type& value)
    {
        const auto r = internal::process_validate<result::integer>::run(connection, "RPUSH %s %b", name.c_str(), value.data(), value.size());
               
        return r.message() > 0;        
    }

    bool rarray::push_front(const string_type& value)
    {
        const auto r = internal::process_validate<result::integer>::run(connection, "LPUSH %s %b", name.c_str(), value.data(), value.size());
               
        return r.message() > 0;
    }

    rarray::result_type rarray::operator [] (size_type index) const
    {
        return at(index);
    }

    rarray::result_type rarray::at(size_type at) const
    {
        const auto r = internal::process_validate<result::array>::run(connection, "GET %s", name.c_str());
        
        const auto st = result::try_into<result::string>(r.operator[](at)).and_then([](const auto& s) -> ::result<std::string, std::string> {
            return ok(result::to_string(s));
        });
        if (st.is_ok()) {
            return st.unwrap();
        } else {
            throw std::runtime_error("this is not a valid message: " + st.error_value());
        }
        assert(false);
        return {};  // we should not be here!!
    }

    rarray::iterator rarray::begin() const
    {
        auto r = internal::process_validate<result::array>::run(connection, "LRANGE %s 0 -1", name.c_str());
        
        reply_iterator i(std::move(r));    
        return i;
    }

    rarray::iterator rarray::end() const
    {
        return {};
    }

    rarray::size_type rarray::size() const
    {
        auto r = internal::process_validate<result::array>::run(connection, "LLEN %s", name.c_str());
        
        return r.size();
    }

    bool rarray::empty() const
    {
        return size() == 0;
    }

    void rarray::erase()
    {
        if (!connection) {
            throw connection_error("trying to use invalid endpoint object to delete value");
        }

        redisCommand(cast(connection), "DEL %s", name.c_str());
    }

}   // end of redis namespace




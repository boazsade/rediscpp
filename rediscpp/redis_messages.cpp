#include "redis_messages.h"
#include "results.h"
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
        if (!boost::algorithm::iequals(em, "ok")) {
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
<<<<<<< HEAD
        // if (!connection) {
        //     throw connection_error("trying to use invalid endpoint object to set int value");
        // }
        // if (const auto r  = Ok(result::try_into<result::integer>(
        //         result::any::from(
        //             (redisReply*)redisCommand(cast(connection), "INCRBY %s %s", name.c_str(), std::to_string(by).c_str())))); r) {
        //     return r.value().message();
        // }
        // throw std::runtime_error("failed to get integer value");
        const auto r = process<result::integer>::run(connection, "INCRBY %s %s", name.c_str(), std::to_string(by).c_str());
        if (r.is_error()) {
            throw connection_error(r.error_value());
        }        
        return r.unwrap().message();        
=======
        const auto r = internal::process_validate<result::integer>::run(connection, "INCRBY %s %s", name.c_str(), std::to_string(by).c_str());
             
        return r.message();        
>>>>>>> e4858b8defad800a2de7ef2bb7e3f6bf2805622c
    }
    
    long_int::value_type long_int::operator -= (value_type by) const
    {
<<<<<<< HEAD
        // if (!connection) {
        //     throw connection_error("trying to use invalid endpoint object to set int value");
        // }
        // if (const auto r  = Ok(result::try_into<result::integer>(
        //         result::any::from(
        //             (redisReply*)redisCommand(cast(connection), "DECRBY %s %s", name.c_str(), std::to_string(by).c_str())))); r) {
        //     return r.value().message();
        // }
        // throw std::runtime_error("failed to get integer value");
        const auto r = process<result::integer>::run(connection, "DECRBY %s %s", name.c_str(), std::to_string(by).c_str());
        if (r.is_error()) {
            throw connection_error(r.error_value());
        }        
        return r.unwrap().message();        
=======
        const auto r = internal::process_validate<result::integer>::run(connection, "DECRBY %s %s", name.c_str(), std::to_string(by).c_str());
                
        return r.message();        
>>>>>>> e4858b8defad800a2de7ef2bb7e3f6bf2805622c
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
<<<<<<< HEAD
        // if (!connection) {
        //     throw connection_error("trying to use invalid endpoint object to save new value");
        // }
        // if (const auto r  = Ok(result::try_into<result::integer>(
        //         result::any::from(
        //             (redisReply*)redisCommand(cast(connection), "RPUSH %s %b", name.c_str(), value.data(), value.size())))); r) {
        //     return r.value().message() > 0;
        // }
        // throw std::runtime_error("failed to insert new element");
        const auto r = process<result::integer>::run(connection, "RPUSH %s %b", name.c_str(), value.data(), value.size());
        if (r.is_error()) {
            throw connection_error(r.error_value());
        }        
        return r.unwrap().message() > 0;        
=======
        const auto r = internal::process_validate<result::integer>::run(connection, "RPUSH %s %b", name.c_str(), value.data(), value.size());
               
        return r.message() > 0;        
>>>>>>> e4858b8defad800a2de7ef2bb7e3f6bf2805622c
    }

    bool rarray::push_front(const string_type& value)
    {
<<<<<<< HEAD
        const auto r = process<result::integer>::run(connection, "LPUSH %s %b", name.c_str(), value.data(), value.size());
        if (r.is_error()) {
            throw connection_error(r.error_value());
        }        
        return r.unwrap().message() > 0;        
        // if (!connection) {
        //     throw connection_error("trying to use invalid endpoint object to save new value");
        // }
        // if (const auto r  = Ok(result::try_into<result::integer>(
        //         result::any::from(
        //             (redisReply*)redisCommand(cast(connection), "LPUSH %s %b", name.c_str(), value.data(), value.size())))); r) {
        //     return r.value().message() > 0;
        // }
        // throw std::runtime_error("failed to insert new element");
=======
        const auto r = internal::process_validate<result::integer>::run(connection, "LPUSH %s %b", name.c_str(), value.data(), value.size());
               
        return r.message() > 0;
>>>>>>> e4858b8defad800a2de7ef2bb7e3f6bf2805622c
    }

    rarray::result_type rarray::operator [] (size_type index) const
    {
        return at(index);
    }

    rarray::result_type rarray::at(size_type at) const
    {
<<<<<<< HEAD
        const auto r = process<result::string>::run(connection, "GET %s", name.c_str());
        if (r.is_error()) {
            throw connection_error(r.error_value());
        }        
        return result::to_string(r.unwrap());
        // if (!connection) {
        //     throw connection_error("trying to use invalid endpoint object to read new value");
        // }
        // if (const auto r  = Ok(result::try_into<result::array>(
        //         result::any::from(
        //             (redisReply*)redisCommand(cast(connection), "GET %s", name.c_str())))); r) {
        //     if (r.value().size() > at) {
        //         auto e = r.value()[at];
        //         const auto s = result::try_into<result::string>(e);
        //         if (s.is_ok()) {
        //             return result::to_string(s.unwrap());
        //         }
        //     }
        // }
        // throw std::runtime_error("failed to insert new element");
        // reply r = (redisReply*)redisCommand(cast(connection), "GET %s", name.c_str());
        // if (r && r.what() == reply::ARRAY && at < r.elements()) {
        // }
        //     reply_iterator i(&r);
        //     i += at;
        //     return result_type(i->answer(), i->size());
        // } else {
        //     static const result_type error = result_type();
        //     return error;
        // }
=======
        const auto r = internal::process_validate<result::array>::run(connection, "GET %s", name.c_str());
        
        const auto s = result::try_into<result::string>(r.operator[](at)).and_then([](const auto& s) -> ::result<std::string, std::string> {
            return ok(result::to_string(s));
        });
        if (s.is_ok()) {
            return s.unwrap();
        } else {
            throw std::runtime_error("this is not a valid message: " + s.error_value());
        }
>>>>>>> e4858b8defad800a2de7ef2bb7e3f6bf2805622c
    }

    rarray::iterator rarray::begin() const
    {
<<<<<<< HEAD
        auto r = process<result::array>::run(connection, "LRANGE %s 0 -1", name.c_str());
        if (r.is_error()) {
            throw connection_error(r.error_value());
        }
        reply_iterator i(std::move(r.unwrap()));    
        return i;
        // if (!connection) {
        //     throw connection_error("trying to use invalid endpoint object to read value");
        // }
        // auto a = result::try_into<result::array>(result::any::from((redisReply*)redisCommand(cast(connection), "LRANGE %s 0 -1", name.c_str())));
        // if (a.is_ok()) { 
        //     reply_iterator i(std::move(a.unwrap()));
        //     return i;
        // } else {
        //     return end();
        // }
=======
        auto r = internal::process_validate<result::array>::run(connection, "LRANGE %s 0 -1", name.c_str());
        
        reply_iterator i(std::move(r));    
        return i;
>>>>>>> e4858b8defad800a2de7ef2bb7e3f6bf2805622c
    }

    rarray::iterator rarray::end() const
    {
        return {};
    }

    rarray::size_type rarray::size() const
    {
<<<<<<< HEAD
        auto r = process<result::array>::run(connection, "LLEN %s", name.c_str());
        if (r.is_error()) {
            throw connection_error(r.error_value());
        }
        return r.unwrap().size();
        // if (!connection) {
        //     throw connection_error("trying to use invalid endpoint object to read value length");
        // }
        // //reply r = (redisReply*)redisCommand(cast(connection), "LLEN %s", name.c_str());
        // if (const auto r  = Ok(result::try_into<result::array>(
        //         result::any::from(
        //             (redisReply*)redisCommand(cast(connection), "GET %s", name.c_str())))); r) {
        //     return r.value().size();
        // }
        // throw std::runtime_error("this is not a valid array entry");
=======
        auto r = internal::process_validate<result::array>::run(connection, "LLEN %s", name.c_str());
        
        return r.size();
>>>>>>> e4858b8defad800a2de7ef2bb7e3f6bf2805622c
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




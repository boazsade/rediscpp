#include "redis_messages.h"
#include "results.h"
#include "redis_reply.h"
#include <hiredis/hiredis.h>
#include <boost/lexical_cast.hpp>
#include <iostream>


namespace redis
{
    namespace 
    {
        template<typename ...Args>
        auto run_op(redis::end_point& endpoint, const char* command, Args...args) -> ::result<result::any, std::string> {
            using namespace std::string_literals;

            if (!connection) {
                return failed("not connected"s);
            }
            auto out = result::any::from(
                    (const redisReply*)std::invoke(redisCommand, cast(connection),
                        command,
                        std::forward<decltype<args>(args)...
                ));
            if (out.is_error()) {
                return failed("redis error: "s + result::try_into<result::error>(out).unwrap().message());
            }
            return ok(std::move(out));
        }

        template<typename Result>
        struct process {
            template<typename ...Args>
            static auto run(redis::end_point& endpoint, const char* command, Args...args) -> ::result<Result, std::string> {
                const auto r = 
                    std::invoke(run_op, end_point, 
                                command, 
                                std::forward<decltype<args>(args)...).
                                    and_then([](auto&& res) -> ::result<Result, std::string> {
                    const auto a = result::try_into<Result>(res.unwrap());
                    return a;
                });
                return r;
            }
        };
        template<>
        struct process<void> {
            template<typename ...Args>
            static auto run(redis::end_point& endpoint, const char* command, Args...args) -> ::result<bool, std::string> {
                const auto r = std::invoke(run_op, end_point, command, ...args);
                if (r.is_error()) {
                    return failed(r.error_value());
                }
                return ok(true);    // place holder
            }
        };
    }   // end of local namespace

    rstring::rstring(end_point ep, const std::string& name) : key_name(name), connection(ep)
    {
    }

    rstring& rstring::operator = (const std::string& value)
    {
        // if (connection) {
        //     redisCommand(cast(connection), "SET %s %b", key_name.c_str(), value.data(), value.size());
        // } else {
        //     throw connection_error("trying to use invalid endpoint object to set string value");
        // }
        // const auto r = run_op(connection,  "SET %s %b", key_name.c_str(), value.data(), value.size());
        // if (r.is_error()) {
        //     throw connection_error(r.error_value());
        // }
        if (const auto e = Error(process<void>::run(connection,  "SET %s %b", key_name.c_str(), value.data(), value.size())); e) {
            throw connection_error(e.value());
        }
        return *this;
    }

    rstring& rstring::operator +=(const std::string& add_str)
    {
        append(add_str);
        return *this;
    }

    void rstring::append(const std::string& add_str)
    {
        // if (connection) {
        //     redisCommand(cast(connection), "APPEND %s %b", key_name.c_str(), add_str.data(), add_str.size());
        // } else {
        //     throw connection_error("trying to use invalid endpoint object to set string value");
        // }
        // const auto r = run_op(connection,"APPEND %s %b", key_name.c_str(), add_str.data(), add_str.size());
        // if (r.is_error()) {
        //     throw connection_error(r.error_value());
        // }
        if (const auto e = Error(process<void>::run(connection,  "APPEND %s %b", key_name.c_str(), add_str.data(), add_str.size())); e) {
            throw connection_error(e.value());
        }
    }

    std::string rstring::str() const
    {
        const auto r = process<result::string>::run(connection, "GET %s", key_name.c_str());
        if (r.is_error()) {
            throw connection_error(r.error_value());
        }
        return result::to_string(r.unwrap());
        // const auto r = run_op(connection,"GET %s", key_name.c_str());
        // if (r.is_error()) {
        //     throw connection_error(r.error_value());
        // }
        // const auto a = result::try_into<result::string>(r.unwrap());
        // if (a.is_error()) {
        //     throw std::runtime_error("invalid result: " + a.error_value());
        // }
        // return result::to_string(a.unwrap());
        // static const std::string error = {};

        // if (connection) {
        //     //reply r = 
        //     const auto r = result::try_into<result::string>(
        //         result::any::from((const redisReply*)redisCommand(cast(connection), 
        //                                 "GET %s", key_name.c_str()
        //                     )
        //     ));

        //     if (Ok(r)) {
        //         return result::to_string(r.unwrap());
        //     }
        // } else {
        //     throw connection_error("trying to use invalid endpoint object to get string value");
        // } 
        // return error;
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
        const auto r = process<result::string>::run(connection, "GETRANGE %s %s %s", key_name.c_str(),
            std::to_string(from).c_str(),
            std::to_string(to).c_str()
        );
        if (r.is_error()) {
            throw connection_error(r.error_value());
        }
        // const auto a = result::try_into<result::string>(r.unwrap());
        // if (a.is_error()) {
        //     throw std::runtime_error("invalid result: " + a.error_value());
        // }
        return result::to_string(r.unwrap());
        // if (!connection) {
        //     throw connection_error("trying to use invalid endpoint object to get string value");
        // }
        // // reply r = (redisReply*)redisCommand(cast(connection), "GETRANGE %s %s %s", key_name.c_str(),
        // //                         boost::lexical_cast<std::string>(from).c_str(), 
        // //                         boost::lexical_cast<std::string>(to).c_str());
        // // if (r && r.answer()) {
        // //     return std::string(r.answer(), r.size());
        // // } else {
        // //     return std::string();
        // // }
        // const auto r = result::try_into<result::string>(
        //         result::any::from((const redisReply*)redisCommand(cast(connection), 
        //                                 "GETRANGE %s %s %s", key_name.c_str(),
        //                                 std::to_string(from).c_str(),
        //                                 std::to_string(to).c_str()
        //                     )
        //     ));

        //     if (Ok(r)) {
        //         return result::to_string(r.unwrap());
        //     }
        //     return {};
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
        const auto r = process<result::integer>::run(connection, "SET %s %b", 
                     key.c_str(), value.data(), value.size());
        if (r.is_error()) {
            throw connection_error(r.error_value());
        }        
        return r.unwrap().message() != 1;
        // if (!connection) {
        //     throw connection_error("trying to use invalid endpoint object to get map entry");
        // }        
        // if (const auto r = Ok(result::try_into<result::integer>(
        //     result::any::from(
        //             (redisReply*)redisCommand(cast(connection), "SET %s %b", 
        //             key.c_str(), value.data(), value.size()
        //         )))); r) {
        //             return r.value().message() != 1;
        //     } else {
        //         return false;
        //     }
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
        // reply r = (redisReply*)redisCommand(cast(connection), "GET %s", key.c_str());
        // if (r && r.what() == reply::STRING) {
        //     return mapped_type(r.answer(), r.size());
        // } else {
            
        //     static const mapped_type error = mapped_type();
        //     return error;
        // }
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
        const auto r = process<result::integer>::run(connection, "DBSIZE");
        if (r.is_error()) {
            throw connection_error(r.error_value());
        }        
        return r.unwrap().message();
        // if (connection) {
        //     if (const auto r = Ok(
        //         result::try_into<result::integer>(
        //             result::any::from((redisReply*)redisCommand(cast(connection), "DBSIZE"))
        //         )
        //     )) {
        //         return r.value().message();
        //     }
        // }
        // return 0;
            // reply r = (redisReply*)redisCommand(cast(connection), "DBSIZE");    // note that this is not for a given entry!!
            // if (r && r.what() == reply::INTEGER) {
            //     return r.get();
            // } else {
            //     return 0;
            // }
        // } else {
        //     return 0;
        // }
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

    long_int::long_int(end_point c, const std::string& n, value_type v) : connection(c), name(n)
    {
        const auto r = process<result::integer>::run(connection, "SET %s %s", name.c_str(), std::to_string(v).c_str());
        if (r.is_error()) {
            throw connection_error(r.error_value());
        }     
        // if (!connection) {
        //     throw connection_error("trying to use invalid endpoint object to set int value");
        // }

        // redisCommand(cast(connection), "SET %s %s", name.c_str(), boost::lexical_cast<std::string>(v).c_str());
    }

    long_int& long_int::operator = (value_type val)
    {
        const auto r = process<result::integer>::run(connection, "SET %s %s", name.c_str(), std::to_string(val).c_str());
        if (r.is_error()) {
            throw connection_error(r.error_value());
        }
        return *this;
    }
    
    long_int::operator long_int::value_type () const
    {
        // if (!connection) {
        //     throw connection_error("trying to use invalid endpoint object to set int value");
        // }
        // if (const auto r  = Ok(result::try_into<result::integer>(
        //         result::any::from(
        //             (redisReply*)redisCommand(cast(connection), "GET %s", name.c_str())))); r) {
        //     return r.value().message();

        // }

        // throw std::runtime_error("failed to get integer value");
        const auto r = process<result::integer>::run(connection, "GET %s", name.c_str());
        if (r.is_error()) {
            throw connection_error(r.error_value());
        }
        
        return r.unwrap().message();
    }

    long_int::value_type long_int::operator ++ () const
    {
        // if (!connection) {
        //     throw connection_error("trying to use invalid endpoint object to set int value");
        // }
        // if (const auto r  = Ok(result::try_into<result::integer>(
        //         result::any::from(
        //             (redisReply*)redisCommand(cast(connection), "INCR %s", name.c_str())))); r) {
        //     return r.value().message();

        // }

        // throw std::runtime_error("failed to get integer value");
        const auto r = process<result::integer>::run(connection, "INCR %s", name.c_str());
        if (r.is_error()) {
            throw connection_error(r.error_value());
        }        
        return r.unwrap().message();
    }
    
    long_int::value_type long_int::operator ++ (int) const
    {
        auto i = this->operator()();
        
        redisCommand(cast(connection), "INCR %s", name.c_str());
        return i;
    }

    long_int::value_type long_int::operator -- () const
    {
        // if (!connection) {
        //     throw connection_error("trying to use invalid endpoint object to set int value");
        // }
        // if (const auto r  = Ok(result::try_into<result::integer>(
        //         result::any::from(
        //             (redisReply*)redisCommand(cast(connection), "DECR %s", name.c_str())))); r) {
        //     return r.value().message();
        // }
        // throw std::runtime_error("failed to get integer value");        
        const auto r = process<result::integer>::run(connection, "DECR %s", name.c_str());
        if (r.is_error()) {
            throw connection_error(r.error_value());
        }        
        return r.unwrap().message();
    }
    
    long_int::value_type long_int::operator -- (int) const
    {
        auto i = this->operator()();
        
        redisCommand(cast(connection), "DECR %s", name.c_str());
        return i;
    }
        
    long_int::value_type long_int::operator += (value_type by) const
    {
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
    }
    
    long_int::value_type long_int::operator -= (value_type by) const
    {
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
    }

    bool rarray::push_front(const string_type& value)
    {
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
    }

    rarray::result_type rarray::operator [] (size_type index) const
    {
        return at(index);
    }

    rarray::result_type rarray::at(size_type at) const
    {
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
    }

    rarray::iterator rarray::begin() const
    {
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
    }

    rarray::iterator rarray::end() const
    {
        return {};
    }

    rarray::size_type rarray::size() const
    {
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




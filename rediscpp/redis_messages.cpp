#include "redis_messages.h"
#include "redis_reply.h"
#include <hiredis/hiredis.h>
#include <boost/lexical_cast.hpp>
#include <iostream>

namespace redis
{
    rstring::rstring(end_point ep, const std::string& name) : key_name(name), connection(ep)
    {
    }

    rstring& rstring::operator = (const std::string& value)
    {
        if (connection) {
            redisCommand(cast(connection), "SET %s %b", key_name.c_str(), value.data(), value.size());
        } else {
            throw connection_error("trying to use invalid endpoint object to set string value");
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
        if (connection) {
            redisCommand(cast(connection), "APPEND %s %b", key_name.c_str(), add_str.data(), add_str.size());
        } else {
            throw connection_error("trying to use invalid endpoint object to set string value");
        }
    }

    std::string rstring::str() const
    {
        static const std::string error = std::string();

        if (connection) {
            reply r = (redisReply*)redisCommand(cast(connection), "GET %s", key_name.c_str());

            if (r && r.answer()) {
                return std::string(r.answer(), r.size());
            }
        } else {
            throw connection_error("trying to use invalid endpoint object to get string value");
        } 
        return error;
    }
    
    std::string::size_type rstring::size() const
    {
        if (connection) {
            reply r = (redisReply*)redisCommand(cast(connection), "GET %s", key_name.c_str());

            if (r) {
                return r.size();
            }
        } else {
            throw connection_error("trying to use invalid endpoint object to get string value");
        }

        return 0;

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
        if (!connection) {
            throw connection_error("trying to use invalid endpoint object to get string value");
        }
        reply r = (redisReply*)redisCommand(cast(connection), "GETRANGE %s %s %s", key_name.c_str(),
                                boost::lexical_cast<std::string>(from).c_str(), 
                                boost::lexical_cast<std::string>(to).c_str());
        if (r && r.answer()) {
            return std::string(r.answer(), r.size());
        } else {
            return std::string();
        }
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
        if (!connection) {
            throw connection_error("trying to use invalid endpoint object to get map entry");
        }        
        reply r = (redisReply*)redisCommand(cast(connection), "SET %s %b", key.c_str(), value.data(), value.size());
        return r && r.get() != 0;
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
        reply r = (redisReply*)redisCommand(cast(connection), "GET %s", key.c_str());
        if (r && r.what() == reply::STRING) {
            return mapped_type(r.answer(), r.size());
        } else {
            
            static const mapped_type error = mapped_type();
            return error;
        }
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
        if (connection) {
            reply r = (redisReply*)redisCommand(cast(connection), "DBSIZE");    // note that this is not for a given entry!!
            if (r && r.what() == reply::INTEGER) {
                return r.get();
            } else {
                return 0;
            }
        } else {
            return 0;
        }
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
        if (!connection) {
            throw connection_error("trying to use invalid endpoint object to set int value");
        }

        redisCommand(cast(connection), "SET %s %s", name.c_str(), boost::lexical_cast<std::string>(v).c_str());
    }

    long_int& long_int::operator = (value_type val)
    {
        if (!connection) {
            throw connection_error("trying to use invalid endpoint object to set int value");
        }
        redisCommand(cast(connection), "SET %s %s", name.c_str(), boost::lexical_cast<std::string>(val).c_str());
        return *this;
    }
    
    long_int::operator long_int::value_type () const
    {
        if (!connection) {
            throw connection_error("trying to use invalid endpoint object to set int value");
        }
        reply r = (redisReply*)redisCommand(cast(connection), "GET %s", name.c_str());
        return r.get();
    }

    long_int::value_type long_int::operator ++ () const
    {
        if (!connection) {
            throw connection_error("trying to use invalid endpoint object to set int value");
        }
        reply r = (redisReply*)redisCommand(cast(connection), "INCR %s", name.c_str());
        return r.get();
    }
    
    long_int::value_type long_int::operator ++ (int) const
    {
        if (!connection) {
            throw connection_error("trying to use invalid endpoint object to set int value");
        }
        reply r = (redisReply*)redisCommand(cast(connection), "GET %s", name.c_str());
        value_type v = r.get();
        redisCommand(cast(connection), "INCR %s", name.c_str());
        return v;
    }

    long_int::value_type long_int::operator -- () const
    {
        if (!connection) {
            throw connection_error("trying to use invalid endpoint object to set int value");
        }
        reply r = (redisReply*)redisCommand(cast(connection), "DECR %s", name.c_str());
        return r.get();
    }
    
    long_int::value_type long_int::operator -- (int) const
    {
        if (!connection) {
            throw connection_error("trying to use invalid endpoint object to set int value");
        }
        reply r = (redisReply*)redisCommand(cast(connection), "GET %s", name.c_str());
        value_type v = r.get();
        redisCommand(cast(connection), "DECR %s", name.c_str());
        return v;
    }
        
    long_int::value_type long_int::operator += (value_type by) const
    {
        if (!connection) {
            throw connection_error("trying to use invalid endpoint object to set int value");
        }
        reply r = (redisReply*)redisCommand(cast(connection), "INCRBY %s %s", name.c_str(), boost::lexical_cast<std::string>(by).c_str());
        return r.get();
    }
    
    long_int::value_type long_int::operator -= (value_type by) const
    {
        if (!connection) {
            throw connection_error("trying to use invalid endpoint object to set int value");
        }
        reply r = (redisReply*)redisCommand(cast(connection), "DECRBY %s %s", name.c_str(), boost::lexical_cast<std::string>(by).c_str());
        return r.get();
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
        if (!connection) {
            throw connection_error("trying to use invalid endpoint object to save new value");
        }
        reply r = (redisReply*)redisCommand(cast(connection),"RPUSH %s %b", name.c_str(), value.data(), value.size());
        return r.valid() && r.what() != reply::ERROR_MSG;
    }

    bool rarray::push_front(const string_type& value)
    {
        if (!connection) {
            throw connection_error("trying to use invalid endpoint object to save new value");
        }
        reply r = (redisReply*)redisCommand(cast(connection),"LPUSH %s %b", name.c_str(), value.data(), value.size());
        return r.valid() && r.what() != reply::ERROR_MSG;
    }

    rarray::result_type rarray::operator [] (size_type index) const
    {
        return at(index);
    }

    rarray::result_type rarray::at(size_type at) const
    {
        if (!connection) {
            throw connection_error("trying to use invalid endpoint object to read new value");
        }
        reply r = (redisReply*)redisCommand(cast(connection), "GET %s", name.c_str());
        if (r && r.what() == reply::ARRAY && at < r.elements()) {
            reply_iterator i(&r);
            i += at;
            return result_type(i->answer(), i->size());
        } else {
            static const result_type error = result_type();
            return error;
        }
    }

    rarray::iterator rarray::begin() const
    {
        if (!connection) {
            throw connection_error("trying to use invalid endpoint object to read value");
        }
        reply r = (redisReply*)redisCommand(cast(connection), "LRANGE %s 0 -1", name.c_str());
        if (r && r.what() == reply::ARRAY) { 
            reply_iterator i(&r);
            return i;
        } else {
            return end();
        }
    }

    rarray::iterator rarray::end() const
    {
        return reply_iterator(0);
    }

    rarray::size_type rarray::size() const
    {
        if (!connection) {
            throw connection_error("trying to use invalid endpoint object to read value length");
        }
        reply r = (redisReply*)redisCommand(cast(connection), "LLEN %s", name.c_str());

        return r.get(); // the length of the array is the integer value we got from the action
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

        reply r = (redisReply*)redisCommand(cast(connection), "DEL %s", name.c_str());
    }

}   // end of redis namespace




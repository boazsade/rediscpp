#include "redis_endpoint.h"
#include <hiredis/hiredis.h>

#ifdef WIN32
#   include <Winsock2.h>
#else   // not WIN32
#   include <sys/time.h>
#endif  // not WIN32

namespace redis
{

namespace
{
    void free_connection(redisContext* rc)
    {
        if (rc) {
            redisFree(rc);
        }
    }

    redisContext* connect(const std::string& host, unsigned short port, const end_point::timeout& to)
    {
        static const unsigned int milli2micro = 1000;

        struct timeval timeout = {cast(to.sec()), cast(to.millis())*milli2micro };
        redisContext* c =  redisConnectWithTimeout(host.c_str(), port, timeout);
        return c;
    }


}   // end of local namespace

end_point::seconds::seconds(unsigned int v) : value(v)
{
}


end_point::seconds& end_point::seconds::operator = (unsigned int v)
{
    value = v;
    return *this;
}

end_point::milliseconds::milliseconds(unsigned int v) : value(v)
{
}

end_point::milliseconds& end_point::milliseconds::operator = (unsigned int v)
{
    value = v;
    return *this;
}

end_point::timeout::timeout(seconds x, milliseconds y) : s(x), ms(y)
{
}

end_point::seconds& end_point::timeout::sec()
{
    return s;
}

const end_point::seconds& end_point::timeout::sec() const
{
    return s;
}

const end_point::milliseconds& end_point::timeout::millis() const
{
    return ms;
}

end_point::milliseconds& end_point::timeout::millis()
{
    return ms;
}

end_point::end_point(const std::string& host, unsigned short port)
{
    open(host, port);
}
end_point::end_point()
{
}

end_point::end_point(const std::string& host, seconds s, unsigned short port)
{
    open(host, s, port);
}

end_point::end_point(const std::string& host, milliseconds ms, unsigned short port)
{
    open(host, ms, port);
}

end_point::end_point(const std::string& host, timeout to, unsigned short port)
{
    open(host, to, port);
}

end_point::~end_point()
{
}

bool end_point::start()
{
    if (connection) {
        close_it();
    }

    if (connection) {   // we still have the connection open
        return false;
    }
   
    return true;
}

bool end_point::open(const std::string& host, unsigned short port)
{
    if (start()) {
    // open the connection to the server
        redisContext* c = redisConnect(host.c_str(), port);
        return finilized(c);
    } else {
        return false;
    }

}

bool end_point::open(const std::string& host, seconds s, unsigned short port)
{
    if (start()) {
    // open the connection to the server
        return finilized(connect(host, port, timeout(s, milliseconds(0))));
    } else {
        return false;
    }

}

bool end_point::open(const std::string& host, milliseconds ms, unsigned short port)
{
    if (start()) {
    // open the connection to the server
        return finilized(connect(host, port, timeout(seconds(0), ms)));
    } else {
        return false;
    }

}

bool end_point::open(const std::string& host, timeout to, unsigned short port)
{
    if (start()) {
    // open the connection to the server
        return finilized(connect(host, port, to));
    } else {
        return false;
    }

}

bool end_point::finilized(redisContext* rc)
{
    // make sure that we don't have error before using this connection
    if (!rc || rc->err) {
        return false;
    } else {
        connection.reset(rc, free_connection);
        return true;
    }

}

end_point::operator boolean_type () const
{
    return connection.get() != 0 ? &end_point::dummy : (boolean_type)0;
}

void end_point::close_it()
{
    connection.reset((redisContext*)0, free_connection);
}

void end_point::set_timeout(const timeout& to)
{
    static const unsigned int milli2micro = 1000;

    if (connection.get()) {
        struct timeval timeout = {cast(to.sec()), cast(to.millis())*milli2micro };
        redisSetTimeout(connection.get(), timeout);
    }
}

connection_error::connection_error(const std::string& err) : std::runtime_error(err)
{
}

}   // end of redis namepsace


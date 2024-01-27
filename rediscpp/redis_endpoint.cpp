#include "redis_endpoint.h"
#include <hiredis/hiredis.h>

#ifdef WIN32
#   include <Winsock2.h>
#else   // not WIN32
#   include <sys/time.h>
#endif  // not WIN32

namespace redis
{

using namespace std::string_literals;
namespace
{

    auto free_connection(redisContext* rc) -> void
    {
        if (rc) {
            redisFree(rc);
        }
    }

    auto connect(const std::string& host, unsigned short port, const end_point::timeout_t& to) -> redisContext*
    {
        struct timeval t = { to.sec().count(),  to.microseconds() };
        redisContext* c =  redisConnectWithTimeout(host.c_str(), port, t);
        return c;
    }


}   // end of local namespace


end_point::end_point(const std::string& host, unsigned short port)
{
    if (const auto e = Error(open(host, port)); e) {
        throw connection_error{e.value()};
    }
}


end_point::end_point(const std::string& host, seconds_t s, unsigned short port)
{
    if (const auto e = Error(open(host, s, port)); e) {
        throw connection_error{e.value()};
    }
}

end_point::end_point(const std::string& host, milliseconds_t ms, unsigned short port)
{
    if (const auto e = Error(open(host, ms, port)); e) {
        throw connection_error{e.value()};
    }
}

end_point::end_point(const std::string& host, timeout_t to, unsigned short port)
{
    if (const auto e = Error(open(host, to, port)); e) {
        throw connection_error{e.value()};
    }
}

end_point::end_point(named_pipe_t pipe) {
     if (const auto e = Error(open(pipe)); e) {
        throw connection_error{e.value()};
    }
}

auto end_point::start() -> result_t
{
    if (connection) {
        close_it();
    }

    if (connection) {   // we still have the connection open
        return failed("error: connection is open"s);
    }
   
    return ok(true);
}

auto end_point::open(const std::string& host, unsigned short port) -> result_t
{
    if (const auto e = Error(start()); e) {
        return failed(e.value());
    }
    // open the connection to the server
    redisContext* c = redisConnect(host.c_str(), port);
    return finalized(c);

}

auto end_point::open(const std::string& host, seconds_t s, unsigned short port) -> result_t
{
    if (const auto e = Error(start()); e) {
        return failed(e.value());
    }
    // open the connection to the server
    return finalized(connect(host, port, timeout_t(s, milliseconds_t(0))));
}

auto end_point::open(const std::string& host, milliseconds_t ms, unsigned short port) -> result_t
{
    if (const auto e = Error(start()); e) {
        return failed(e.value());
    }
    // open the connection to the server
    return finalized(connect(host, port, timeout_t(seconds_t(0), ms)));
}

auto end_point::open(const std::string& host, timeout_t to, unsigned short port) -> result_t
{
    if (const auto e = Error(start()); e) {
        return failed(e.value());
    }
    // open the connection to the server
    return finalized(connect(host, port, to));
}

auto end_point::finalized(redisContext* rc) -> result_t
{
    // make sure that we don't have error before using this connection
    if (!rc || rc->err) {
        return failed("we are in invalid state - cannot start connection"s);
    } else {
        connection.reset(rc, free_connection);
        return ok(true);
    }

}

end_point::operator boolean_type () const
{
    return connection.get() != 0 ? &end_point::dummy : (boolean_type)0;
}

auto end_point::close_it() -> void
{
    finalized(nullptr);
}

auto end_point::open(named_pipe_t pipe) -> result_t {
    timeval tv;
    tv.tv_sec = pipe.timeout_val.sec().count();
    tv.tv_usec = pipe.timeout_val.microseconds();
    auto r = redisConnectUnixWithTimeout(pipe.name.c_str(), tv);
    if (r) {
        connection.reset(r, free_connection);
    }
    switch (r != nullptr) {
        case true:
            return ok(true);
        case false:
            return failed("failed to initiate connection over local pipe " + pipe.name);
    }
}

auto end_point::set_timeout(const timeout_t& to) -> void
{
    if (connection.get()) {
        struct timeval t = { (to.sec().count()), to.microseconds() };
        redisSetTimeout(connection.get(), t);
    }
}

connection_error::connection_error(const std::string& err) : std::runtime_error(err)
{
}

}   // end of redis namespace


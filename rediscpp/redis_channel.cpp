#include "redis_channel.h"
#include "redis_reply.h"
#include <hiredis/hiredis.h>
#include <errno.h>

namespace redis
{
    namespace
    {
        const std::string TERMINATION_MESSAGE = std::string("TErMinate++&%$$@@^jjk&5");  // some random string..
    }   // end of local message


    channel::channel()
    {
    }

    channel::channel(const std::string& n, end_point srv) : server(srv), topic(n)
    {
    }

    bool channel::create(const std::string& n, end_point srv)
    {
        server = srv;
        topic = n;
        return true;
    }


    const char* channel::name() const
    {
        return topic.c_str();
    }

    end_point& channel::by() const
    {
        return server;
    }

    publisher channel::make_publisher() const
    {
        if (server && !topic.empty()) {
            return publisher(*this);
        } else {
            throw connection_error("invalid use of redis endpoint object");
            return publisher(*this);
        }
    }

    subscriber channel::make_subscriber() const
    {
        if (server && !topic.empty()) {
            return subscriber(*this);
        } else {
            throw connection_error("invalid use of redis endpoint object");
            return subscriber(*this);
        }
    }
#if defined (WIN32) && defined(close)
#   pragma push_macro("close")
#   undef close
#   define IGNORE_DEFINE_FOR_CLOSE_GLOBAL_LEVEL_WAS_PUSHED
#endif  // WIN32 && close
    void channel::close()
    {
        server.close_it();
        topic = std::string();
    }
#if defined (IGNORE_DEFINE_FOR_CLOSE_GLOBAL_LEVEL_WAS_PUSHED)
#   pragma pop_macro("close")
#   undef IGNORE_DEFINE_FOR_CLOSE_GLOBAL_LEVEL_WAS_PUSHED
#endif  // WIN32 && close
    ///////////////////////////////////////////////////////////////////////////
    //

    publisher::publisher(const channel& c) : comm(c)
    {
    }

    publisher::~publisher()
    {
    }

    void publisher::send(const std::string& msg) const
    {
        if (comm.by()) {
            redisCommand(cast(comm.by()), "PUBLISH %s %b", comm.name(), msg.data(), msg.size());
        } else {
            throw connection_error("trying to send with invalid redis endpoint object");
        }

    }

    ///////////////////////////////////////////////////////////////////////////
    //
    subscriber::subscriber(const channel& c) : comm(c), done(false)
    {
        if (!comm.by()) {
            throw connection_error("trying to create subscriber with invalid redis endpoint object");
        }
        redisCommand(cast(comm.by()), "SUBSCRIBE %s", comm.name());
    }

    subscriber::~subscriber()
    {
#if defined (WIN32) && defined(close)
#   pragma push_macro("close")
#   undef close
#   define IGNORE_DEFINE_FOR_CLOSE_GLOBAL_LEVEL_WAS_PUSHED
#endif  // WIN32 && close
        close();
#if defined (IGNORE_DEFINE_FOR_CLOSE_GLOBAL_LEVEL_WAS_PUSHED)
#   pragma pop_macro("close")
#   undef IGNORE_DEFINE_FOR_CLOSE_GLOBAL_LEVEL_WAS_PUSHED
#endif  // IGNORE_DEFINE_FOR_CLOSE_GLOBAL_LEVEL_WAS_PUSHED
    }

#if defined (WIN32) && defined(close)
#   pragma push_macro("close")
#   undef close
#   define IGNORE_DEFINE_FOR_CLOSE_GLOBAL_LEVEL_WAS_PUSHED
#endif  // WIN32 && close
    void subscriber::close() const
    {
#if defined (IGNORE_DEFINE_FOR_CLOSE_GLOBAL_LEVEL_WAS_PUSHED)
#   pragma pop_macro("close")
#   undef IGNORE_DEFINE_FOR_CLOSE_GLOBAL_LEVEL_WAS_PUSHED
#endif  // IGNORE_DEFINE_FOR_CLOSE_GLOBAL_LEVEL_WAS_PUSHED
        if (!comm.by()) {
            throw connection_error("trying to close subscriber with invalid redis endpoint object");
        }
        redisCommand(cast(comm.by()), "UNSUBSCRIBE %s", comm.name());
        done = true;
    }

    subscriber::message_type subscriber::read() const
    {
#if defined(ERROR)
#   undef ERROR
#endif
        static const message_type error = message_type(std::string(), ERROR);
        static const message_type timeout = message_type(std::string(), TIME_OUT);
        static const message_type finish = message_type(std::string(), DONE);

        if (!comm.by()) {
            throw connection_error("trying to close subscriber with invalid redis endpoint object");
        }

        if (done) {
            return finish;
        }
        reply red;

        redisReply* r = cast(red);

        if (redisGetReply(cast(comm.by()), (void**)&r) == REDIS_OK) {
            if (r->element && r->element[2]) {

                std::string msg = std::string(r->element[2]->str, r->element[2]->len); 
                if (msg == TERMINATION_MESSAGE || msg.empty()) {
#if defined (WIN32) && defined(close)
#   pragma push_macro("close")
#   undef close
#   define IGNORE_DEFINE_FOR_CLOSE_GLOBAL_LEVEL_WAS_PUSHED
#endif  // WIN32 && close
                    close();
#if defined (IGNORE_DEFINE_FOR_CLOSE_GLOBAL_LEVEL_WAS_PUSHED)
#   pragma pop_macro("close")
#   undef IGNORE_DEFINE_FOR_CLOSE_GLOBAL_LEVEL_WAS_PUSHED
#endif  //IGNORE_DEFINE_FOR_CLOSE_GLOBAL_LEVEL_WAS_PUSHED
                    return finish;
                } else {
                    return message_type(msg, OK);
                }
            } else {
                int e = errno;
                if (e == EAGAIN || e == EWOULDBLOCK) {
                    return timeout;
                } else {
#if defined (WIN32) && defined(close)
#   pragma push_macro("close")
#   undef close
#   define IGNORE_DEFINE_FOR_CLOSE_GLOBAL_LEVEL_WAS_PUSHED
#endif  // WIN32 && close
                    close();
#if defined (IGNORE_DEFINE_FOR_CLOSE_GLOBAL_LEVEL_WAS_PUSHED)
#   pragma pop_macro("close")
#   undef IGNORE_DEFINE_FOR_CLOSE_GLOBAL_LEVEL_WAS_PUSHED
#endif  // IGNORE_DEFINE_FOR_CLOSE_GLOBAL_LEVEL_WAS_PUSHED
                    return finish;
                }
            }
        } else {
            int e = errno;
            if (e == EAGAIN || e == EWOULDBLOCK) {
                return timeout;
            } else {
                return error;
            }
        }
	// should not get here
	return error;
    }

    subscriber::message_type subscriber::read(const end_point::timeout& to) const
    {
        static const end_point::timeout no_timeout = end_point::timeout(end_point::seconds(0), end_point::milliseconds(0));
        comm.by().set_timeout(to);
        message_type m = read();
        comm.by().set_timeout(no_timeout);
        return m;
    }

    subscriber::message_type subscriber::read(const end_point::seconds& s) const
    {
        return read(end_point::timeout(s, end_point::milliseconds(0)));
    }

    subscriber::message_type subscriber::read(const end_point::milliseconds& ms) const
    {
        return read(end_point::timeout(end_point::seconds(0), ms));
    }

    void subscriber::interrupt() const
    {
        publisher p = comm.make_publisher();
        comm.by().set_timeout(end_point::timeout(end_point::seconds(1), end_point::milliseconds(0)));
        p.send(TERMINATION_MESSAGE); 
    }
}   // end of namespace redis


#pragma once

#include <string>
#include <boost/shared_ptr.hpp>
#include <stdexcept>

/**
 * for more information about what this is doing
 * https://github.com/redis/hiredis
 * but in general this would implement the use of redis message passing
 * and setting for redis protocol
 * you can set a timeout on the connection as well as the port number and the IP to connect to
 * this is the base for all the other classes here to work as you must first connect to the 
 * server to perform any other operation. note that there is no data the is stored outside of the 
 * redis server - so that the only data the needs to be saved is the connection
 **/

struct redisContext;

namespace redis
{

    struct connection_error : public std::runtime_error
    {
        connection_error(const std::string& err); 
    };

    // connecting to the server
    struct end_point
    {
        static const unsigned short DEFAULT_PORT = 6379;


        struct seconds
        {
            explicit seconds(unsigned int v);

            seconds& operator = (unsigned int v);

            friend unsigned int cast(seconds s) {
                return s.value;
            }

        protected:
            unsigned int value;
        };

        struct milliseconds
        {
            explicit milliseconds(unsigned int v);

            milliseconds& operator = (unsigned int v);

            friend unsigned int cast(milliseconds s) {
                return s.value;
            }
        protected:
            unsigned int value;
        };

        struct timeout
        {
            timeout(seconds s, milliseconds ms);

            seconds& sec(); 

            const seconds& sec() const; 

            milliseconds& millis(); 

            const milliseconds& millis() const; 

        private:
            seconds s;
            milliseconds ms;
        };


        end_point();    // default ctor dose nothing

        explicit end_point(const std::string& host, unsigned short port = DEFAULT_PORT);

        end_point(const std::string& host, seconds s, unsigned short port = DEFAULT_PORT);

        end_point(const std::string& host, milliseconds ms, unsigned short port = DEFAULT_PORT);

        end_point(const std::string& host, timeout to, unsigned short port = DEFAULT_PORT);

        ~end_point();

        bool open(const std::string& host, seconds s, unsigned short port = DEFAULT_PORT);

        bool open(const std::string& host, milliseconds ms, unsigned short port = DEFAULT_PORT);

        bool open(const std::string& host, timeout to, unsigned short port = DEFAULT_PORT);

        bool open(const std::string& host, unsigned short port = DEFAULT_PORT);

        void close_it();

        void set_timeout(const timeout& to);

        friend redisContext* cast(end_point& from) {
            if (from) {
                return from.connection.get();
            } else {
                throw connection_error("redis end point object not valid!");
                return (redisContext*)0;
            }
        }

    private:
        void dummy() const {}

        bool finilized(redisContext* rc);

        bool start();

    public:
        typedef void (end_point::*boolean_type)() const;

        operator boolean_type () const;     // return false type (not false of C++) if this has an error

    private:
        typedef boost::shared_ptr<redisContext> data_type;
        data_type connection;
    };
}   // end of namespace redis


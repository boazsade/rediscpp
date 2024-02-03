#pragma once

#include "result/results.h"
#include <string>
#include <memory>
#include <stdexcept>
#include <optional>
#include <filesystem>
#include <tuple>
#include <chrono>



/**
 * for more information about what this is doing
 * https://github.com/redis/hiredis
 * but in general this would implement the use of redis message passing
 * and setting for redis protocol
 * you can set a timeout_t on the connection as well as the port number and the IP to connect to
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
        static constexpr std::uint16_t DEFAULT_PORT = 6379;
        
        using result_t = ::result<bool, std::string>;
        using seconds_t = std::chrono::seconds;
        using milliseconds_t = std::chrono::milliseconds;
        // struct seconds_t
        // {
        //     constexpr seconds_t() = default;

        //     constexpr explicit seconds_t(std::uint32_t v) : value{v} {

        //     }

        //     constexpr auto operator = (std::uint32_t v) -> seconds_t& {
        //         value = v;
        //         return *this;
        //     }

        //     friend constexpr auto cast(seconds_t s) -> std::uint32_t {
        //         return s.value;
        //     }

        //     std::uint32_t value = 0;
        // };

        // struct milliseconds_t
        // {
        //     constexpr milliseconds_t() = default;

        //     constexpr explicit milliseconds_t(std::uint32_t v) : value{v} {

        //     }

        //     constexpr auto operator = (std::uint32_t v) -> milliseconds_t& {
        //         value = v;
        //         return *this;
        //     }

        //     friend constexpr auto cast(milliseconds_t s) -> std::uint32_t {
        //         return s.value;
        //     }
        
        //     std::uint32_t value = 0;
        // };

        struct timeout_t
        {
            constexpr timeout_t() = default;

            constexpr timeout_t(seconds_t s, milliseconds_t ms) : seconds{s}, millis{ms} {

            }

            constexpr auto sec() -> seconds_t& {
                return seconds;
            }

            constexpr auto sec() const -> const seconds_t&  {
                return seconds;
            }

            constexpr auto milliseconds () -> milliseconds_t& {
                return millis;
            }

            constexpr auto microseconds() const -> std::int64_t {
                return milliseconds().count() * 1000L;
            }

            constexpr auto milliseconds () const -> const milliseconds_t& {
                return millis;
            }

        
            seconds_t seconds = {};
            milliseconds_t millis = {};
        };

        inline auto to_string(timeout_t to) -> std::string {
            return "timeout: [" + std::to_string(to.sec().count()) + ":" + std::to_string(to.milliseconds().count()) + "]";
        }

        struct named_pipe_t {
            std::string name;
            timeout_t   timeout_val;

            explicit named_pipe_t(std::string_view n, timeout_t to = {seconds_t{0}, milliseconds_t{0}}) :   // this is GCC/clang bug
                name{n}, timeout_val{std::move(to)} {

            }
        };

        inline auto to_string(named_pipe_t p) -> std::string {
            return "pipe: " + p.name + ", timeout: " + to_string(p.timeout_val);
        }


        end_point() = default;    // default ctor dose nothing

        explicit end_point(const std::string& host, std::uint16_t port = DEFAULT_PORT);

        end_point(const std::string& host, seconds_t s, std::uint16_t port = DEFAULT_PORT);

        end_point(const std::string& host, milliseconds_t ms, std::uint16_t port = DEFAULT_PORT);

        end_point(const std::string& host, timeout_t to, std::uint16_t port = DEFAULT_PORT);

        end_point(named_pipe_t pipe);    // use local unix socket (named pipe)


        auto open(const std::string& host, seconds_t s, std::uint16_t port = DEFAULT_PORT) -> result_t;

        auto open(const std::string& host, milliseconds_t ms, std::uint16_t port = DEFAULT_PORT)  -> result_t;

        auto open(const std::string& host, timeout_t to, std::uint16_t port = DEFAULT_PORT) -> result_t;

        auto open(const std::string& host, std::uint16_t port = DEFAULT_PORT) -> result_t;

        auto open(named_pipe_t pipe) -> result_t;

        auto close_it() -> void;

        auto set_timeout(const timeout_t& to) -> void;

        friend auto cast(end_point& from) -> redisContext* {
            if (from) {
                return from.connection.get();
            } else {
                throw connection_error("redis end point object not valid!");
                return nullptr;
            }
        }

    private:
        auto dummy() const -> void {}

        auto finalized(redisContext* rc) -> result_t;

        auto start() -> result_t;

    public:
        using  boolean_type = void (end_point::*)() const;

        operator boolean_type () const;     // return false type (not false of C++) if this has an error

    private:
        using data_type = std::shared_ptr<redisContext>;
        data_type connection;
    };
}   // end of namespace redis


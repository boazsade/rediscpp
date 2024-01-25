#ifndef REDIS_INTERNAL_IMPL_H
#define REDIS_INTERNAL_IMPL_H
#include "rediscpp/redis_endpoint.h"
#include "rediscpp/redis_reply.h"
#include "result/results.h"
#include <hiredis/hiredis.h>
#include <type_traits>
#include <string>

namespace redis {
    namespace internal {
        template<typename ...Args>
        auto run_op(redis::end_point& endpoint, const char* command, Args...args) -> ::result<result::any, std::string> {
            using namespace std::string_literals;

            if (!endpoint) {
                return failed("not connected"s);
            }
            const auto out = result::any::from(
                    (const redisReply*)std::invoke(redisCommand, cast(endpoint),
                        command,
                        std::forward<decltype(args)>(args)...
                ));
            if (out.is_error()) {
                const auto ev = result::try_into<result::error>(out);
                const auto e = ev.unwrap();
                const auto es = e.message();
                const auto se = std::string(es.data(), es.size());
                return failed("redis error: "s + se);
            }
            return ok(out);
        }

        template<typename Result>
        struct process {
            template<typename ...Args>
            static auto run(redis::end_point& endpoint, const char* command, Args...args) -> ::result<Result, std::string> {
                auto r = 
                    run_op(endpoint, command, 
                            std::forward<decltype(args)>(args)...).
                                    and_then([](auto&& res) -> ::result<Result, std::string> {
                                         return result::try_into<Result>(res);
                                    }
                        );
                return r;
            }
        };
        template<>
        struct process<void> {
            template<typename ...Args>
            static auto run(redis::end_point& endpoint, const char* command, Args...args) -> ::result<bool, std::string> {
                const auto r = //std::invoke(
                    run_op(endpoint, command, std::forward<decltype(args)>(args)...);
                if (r.is_error()) {
                    return failed(r.error_value());
                }
                return ok(true);    // place holder
            }
        };

        template<typename T>
        struct process_validate {
            using result_type = T;

            template<typename ...Args>
            static auto run(redis::end_point& endpoint, const char* command, Args...args) -> result_type {
                const auto r = process<T>::run(endpoint, command, std::forward<decltype(args)>(args)...);
                if (r.is_error()) {
                    throw connection_error(r.error_value());
                }
                if constexpr (std::is_same_v<void, T>) {
                    return;
                } else {
                    return r.unwrap();
                }
            }
        };
    }   // end of namespace internal
}       // end of namespace redis
#else
#   error "you cannot include this inside header file!!"
#endif // REDIS_INTERNAL_IMPL_H
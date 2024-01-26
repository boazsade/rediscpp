#pragma once

#include "result/results.h"
#include <string>
#include <boost/shared_ptr.hpp>
#include <iosfwd>
#include <variant>
#include <string_view>

struct redisReply;

namespace redis
{
namespace result {
    enum class result_type {
        INTEGER, NULL_MESSAGE, STRING, ARRAY, STATUS, ERROR_MSG
    };
    namespace details
    {
        struct lowlevel_access {
            using internals = const redisReply;
        protected:
            explicit lowlevel_access(internals*, int);
            auto get() const -> const internals* {
                return handler.get();
            }

            auto access() const -> internals;
        private:
            static auto free(lowlevel_access me) -> void;
            using internal_handler = std::shared_ptr<internals>;

            internal_handler handler;
        };

        // template<result_type Type>
        // struct data_type {
        //     using internals = lowlevel_access::internals;
        // protected:
        //     data_type(internals* from, int cond) : handler{from} {
        //         panic_if(from, cond);
        //     }

        //     auto access() const -> internals& {
        //         assert(handler.get());
        //         return *handler.get();
        //     }

        //     details::lowlevel_access handler;
        // };
    }       // end of namespace details

    const struct differ_t {} differ = differ_t();
    struct any;
    

    struct array : details::lowlevel_access {
        using base_t = details::lowlevel_access;

        explicit array(base_t::internals*);

        auto empty() const -> bool;
        auto size() const -> std::size_t;
        auto operator [] (std::size_t at) const -> any;
    };

    struct status : details::lowlevel_access  {
        using base_t = details::lowlevel_access;

        explicit status(base_t::internals*);

        auto message() const -> std::string_view;
    };

    struct error : details::lowlevel_access {
        using base_t = details::lowlevel_access;

        explicit error(base_t::internals*);
        auto message() const -> std::string_view;
    };

    struct string : details::lowlevel_access{
        using base_t = details::lowlevel_access;

        explicit string(base_t::internals* from);

        auto message() const -> std::string_view;

        auto empty() const -> bool;

        auto size() const -> std::size_t;
    };

    inline auto to_string(const string& from) -> std::string {
        return {from.message().data(), from.message().size()};
    }

    struct null : details::lowlevel_access {
        using base_t = details::lowlevel_access;

        explicit null(base_t::internals*);
    };

    struct integer : details::lowlevel_access {
        using base_t = details::lowlevel_access;

        explicit integer(base_t::internals*);

        auto message() const -> std::int64_t;
    };

    
    // helper type for the visitor
    template<class... Ts>
    struct overloaded : Ts... { using Ts::operator()...; };
    // explicit deduction guide (not needed as of C++20)
    template<class... Ts>
    overloaded(Ts...) -> overloaded<Ts...>;

    auto to_string(const any& a) -> std::string;
    struct any {

        any() = default;

        static auto from(details::lowlevel_access::internals* input) -> any;

        constexpr auto is_int() const -> bool {
            return std::holds_alternative<integer>(internal);
        }

        constexpr auto is_null() const -> bool {
            return std::holds_alternative<null>(internal);
        }

        constexpr auto is_string() const -> bool {
            return std::holds_alternative<string>(internal);
        }

        constexpr auto is_status() const -> bool {
            return std::holds_alternative<status>(internal);
        }

        constexpr auto is_error() const -> bool {
            return std::holds_alternative<error>(internal);
        }

        constexpr auto is_array() const -> bool {
            return std::holds_alternative<array>(internal);
        }

        auto as_int() const -> ::result<integer, std::string> {
            return cast<integer>([this]() { return is_int(); });
        }

        auto as_string() const -> ::result<string, std::string> {
            return cast<string>([this]() { return is_string(); });
        }

        auto as_null() const -> ::result<null, std::string> {
            return cast<null>([this]() { return is_null(); });
        }

        auto as_error() const -> ::result<error, std::string> {
            return cast<error>([this]() { return is_error(); });
        }

        auto as_status() const -> ::result<status, std::string> {
            return cast<status>([this]() { return is_status(); });
        }

        auto as_array() const -> ::result<array, std::string> {
            return cast<array>([this]() { return is_array();});
        }

            // allow you to run some operation F with Args on the result
            // note that the first arg for F must match the value inside our result
            // for example if we have string type than F must be
            // auto foo(string&&, ...args) -> T
        // template<typename F, typename ...Args>
        // auto run_op(F&& f, Args...args) const -> typename std::invoke_result_t<F, Args...> {
        //     std::visit(overloaded{
        //         [f](const error& arg) { std::forward(f, arg, ...args); },
        //         [f](const status& arg) {std::forward(f, arg, ...args);},
        //         [f](integer arg) { std::forward(f, arg, ...args); },
        //         [](const array& arg) {std::forward(f, arg, ...args); },
        //         [](const string& arg) { std::forward(f, arg, ...args); },
        //         [](null arg) { std::forward(f, arg, ...args); }
        //     }, internal);
        // }


    private:
        any(array&& a) : internal{std::move(a)} {

        }

        any(string&& s) : internal{std::move(s)} {

        }

        any(integer&& i) : internal{std::move(i)} {

        }

        any(status&& s) : internal{std::move(s)} {

        }

        any(error&& s) : internal{std::move(s)} {

        }

        any(null&& s) : internal{std::move(s)} {

        }

        template<typename Out, typename F>
        auto cast(F&& f) const -> ::result<Out, std::string> {
            if (f()) {
                return ok(std::get<Out>(internal));
            }
            return failed("using access to the wrong type from actual type " + to_string(*this));
        }

        using cast_type = std::variant<
                std::monostate,
                integer,
                string,
                array,
                status,
                error,
                null
        >;

        cast_type internal;
    };

    template<typename T>
    auto try_into(const any& from) -> ::result<T, std::string>;

    template<> inline
    auto try_into<integer>(const any& from) -> ::result<integer, std::string> {
        return from.as_int();
    }

    template<> inline
    auto try_into<null>(const any& from) -> ::result<null, std::string> {
        return from.as_null();
    }

    template<> inline
    auto try_into<string>(const any& from) -> ::result<string, std::string> {
        return from.as_string();
    }

    template<> inline
    auto try_into<error>(const any& from) -> ::result<error, std::string> {
        return from.as_error();
    }

    template<> inline
    auto try_into<status>(const any& from) -> ::result<status, std::string> {
        return from.as_status();
    }

    template<>
    auto try_into<array>(const any& from) -> ::result<array, std::string> {
        return from.as_array();
    }

    auto operator << (std::ostream& os, const any& a) -> std::ostream&;
}   // end of namespace result
}   // end of namespace redis


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

            explicit lowlevel_access(internals*);
            constexpr auto get() const -> const internals* {
                return handler.get();
            }
        private:
            static auto free(lowlevel_access me) -> void;
            using internal_handler = std::shared_ptr<internals>;

            internal_handler handler;
        };



        template<result_type Type>
        struct data_type {
            using internals = lowlevel_access::internals;
        protected:
            data_type(internals* from, int cond) : handler{from} {
                assert(from);
                assert(from->type == cond);
            }

            auto access() const -> internals& {
                assert(handle.get());
                return *handler.get();
            }

            details::lowlevel_access handler;
        };
    }       // end of namespace details

    const struct differ_t {} differ = differ_t();
    struct any;
    

    struct array : details::data_type<result_type::ARRAY> {
        using base_t = details::data_type<result_type::ARRAY>;

        explicit array(base_t::internals*);

        auto empty() const -> bool;
        auto size() const -> std::size_t;
        auto operator [] (std::size_t at) const -> any;
    };

    struct status : details::data_type<result_type::STATUS>  {
        using base_t = details::data_type<result_type::STATUS>;

        explicit status(base_t::internals*);

        auto message() const -> std::string_view;
    private:
        details::lowlevel_access handler;
    };

    struct error : details::data_type<result_type::ERROR_MSG> {
        using base_t = details::data_type<result_type::ERROR_MSG>;

        explicit error(base_t::internals*);
        auto message() const -> std::string_view;
    };

    struct string : details::data_type<result_type::STRING> {
        using base_t = details::data_type<result_type::STRING>;

        explicit string(base_t::internals* from);

        auto message() const -> std::string_view;

        auto empty() const -> bool;

        auto size() const -> std::size_t;

    private:
        details::lowlevel_access handler;
    };

    inline auto to_string(const string& from) -> std::string {
        return {from.message().data(), from.message().size()};
    }

    struct null : details::data_type<result_type::NULL_MESSAGE> {
        using base_t = details::data_type<result_type::NULL_MESSAGE>;

        explicit null(base_t::internals*);
    };

    struct integer : details::data_type<result_type::INTEGER> {
        using base_t = details::data_type<result_type::INTEGER>;

        explicit integer(base_t::internals*);

        auto message() const -> std::int64_t;
    };

    
    // helper type for the visitor
    template<class... Ts>
    struct overloaded : Ts... { using Ts::operator()...; };
    // explicit deduction guide (not needed as of C++20)
    template<class... Ts>
    overloaded(Ts...) -> overloaded<Ts...>;

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
        template<typename F, typename ...Args>
        auto run_op(F&& f, Args...args) const -> typename std::invoke_result_t<F, Args...> {
            std::visit(overloaded{
                [](const error& arg) { std::forward(F, arg, ...args); },
                [](const status& arg) {std::forward(F, arg, ...args);},
                [](integer arg) { std::forward(F, arg, ...args); },
                [](const array& arg) {std::forward(F, arg, ...args); },
                [](const string& arg) { std::forward(F, arg, ...args); },
                [](null arg) { std::forward(F, arg, ...args); }
            }, internal);
        }


    private:
        any() = default;
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

        any(array&& s) : internal{std::move(s)} {

        }

        any(null&& s) : internal{std::move(s)} {

        }

        template<typename Out, typename F>
        auto cast(F&& f) const -> ::result<Out, std::string> {
            if (f) {
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

    auto to_string(const any& a) -> std::string;

    template<typename T>
    auto try_into(const any& from) -> ::result<T, std::string> {
        static_assert(false);
    }

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
#if 0
    // this is used for all message passing to the server and from the server
    // note that we need to make sure that we got a reply through operator boolean i.e. if (reply) {
    // otherwise it would failed for every function here
    // this would be used for reading any type of message returned from the server.
    // normally we would not use this explicitly unless we have to get this from the 
    // return value of an iterator
    struct reply
    {

        reply();                    // post condition !reply == true_type

        reply(redisReply* rr);      // post condition reply == true_type       

        ~reply();                   // release - if this is not in use any more then close reply

        reply& operator = (redisReply* rr); // post condition reply == true_type

        const char* answer() const; // only if an error or STRING and STATUS result type otherwise NULL

        std::size_t elements() const;  // only if result_type == ARRAY otherwise 0

        long long get() const;            // only if result_type == INTEGER otherwise MAX_INT

        result_type what() const;   // return what type of results is not !reply

        std::size_t size() const;   // return the length of the message if reply and not ERROR 

        reply operator [] (std::size_t at) const;   // return element in the list of sub elements but only if this is an ARRAY

        bool empty_list() const;        // return true if not ARRAY or no elements were returned

        friend inline redisReply* cast(reply& from) {
            return from.get_ptr();
        }

        friend inline const redisReply* cast(const reply& from) {
            return from.get_ptr();
        }

    protected:
         reply(redisReply* rr, differ_t);    // this would signal that we would not free this object (when iterating over elements

    private:
        using cast_type = std::variant<
                std::monostate,
                std::int64_t,
                std::string_view,
                array,
                status,
                error
            >;

        void dummy() const {}       // see below this is not for use!

        redisReply* get_ptr();

        const redisReply* get_ptr() const;

    public:
        typedef void(reply::*boolean_type)() const; // for casting for boolean operations

        operator boolean_type () const; // return false type (not false of C++) if this has an error

        bool valid() const;

    private:
        typedef boost::shared_ptr<redisReply>   data_type;
        data_type data;
        cast_type user_data;
    };
    std::ostream& operator << (std::ostream& os, const reply& rep);
#endif
}   // end of namespace result
}   // end of namespace redis


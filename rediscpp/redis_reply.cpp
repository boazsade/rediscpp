#include "redis_reply.h"
#include <hiredis/hiredis.h>
#include <limits.h>
#include <string.h>
#include <iostream>


namespace redis
{
namespace result 
{
namespace
{
    void release_reply(const redisReply* r)
    {
        if (r && (r->type != 0 || r->integer != 0 || r->len != 0)) {
            freeReplyObject(const_cast<redisReply*>(r));    // like any "good" C interface this is not const :(
// #ifndef WIN32   // this would trigger run time error about accessing a freed memory on windows!
//             memset(redisReply*)r, 0, sizeof(*r));
// #endif  // WIN32
        }
    }
}   // end of local namespace

namespace details {

auto panic_if(lowlevel_access::internals* from, int cond) -> void {
    assert(from);
    assert(from->type == cond);
}

lowlevel_access::lowlevel_access(internals* from, int cond) : 
    handler(from, release_reply) {
    panic_if(get(), cond);
}

auto lowlevel_access::access() const -> internals {
    assert(get());
    return *get();
}

}   // end of namespace details

auto array::size() const -> std::size_t {
    return access().elements;
}

auto array::empty() const -> bool {
    return size() == 0;
}

auto array::operator[] (std::size_t at) const -> any {
    assert(at < size());
    return any::from(access().element[at]);
}

status::status(base_t::internals* f) : base_t(f, REDIS_REPLY_STATUS) {
}

string::string(base_t::internals* from) : base_t{from, REDIS_REPLY_STRING} {
}

array::array(base_t::internals* from) : base_t{from, REDIS_REPLY_ARRAY} {
}

null::null(base_t::internals* from) : base_t{from, REDIS_REPLY_NIL} {
}

error::error(base_t::internals* from) : base_t{from, REDIS_REPLY_ERROR} {
}

integer::integer(base_t::internals* from) : base_t{from, REDIS_REPLY_INTEGER} {
}

auto status::message() const -> std::string_view {
    return {access().str, access().len};
}

auto error::message() const -> std::string_view {
    return {access().str, access().len};
}

auto integer::message() const -> std::int64_t {
    return access().integer;
}

auto string::message() const -> std::string_view {
    return {access().str, access().len};
}

auto string::empty() const -> bool {
    return message().empty();
}

auto string::size() const -> std::size_t {
    return message().size();
}

auto any::from(details::lowlevel_access::internals* input) -> any {
    if (!input) {
        return any{};
    }

    switch (input->type) {
        case REDIS_REPLY_ARRAY:
            return any{array{input}};
        case REDIS_REPLY_ERROR:
            return any{error{input}};
        case REDIS_REPLY_INTEGER:
            return any{integer{input}};
        case REDIS_REPLY_NIL:
            return any{null{input}};
        case REDIS_REPLY_STATUS:
            return any{status{input}};
        case REDIS_REPLY_STRING:
            return any{string{input}};
        default:
            return any{};
        
    }
}

auto to_string(const any& a) -> std::string {
    using namespace std::string_literals;

    if (a.as_array()) {
        return "array type"s;
    }
    if (a.is_error()) {
        return "error message"s;
    }

    if (a.is_int()) {
        return "integer type"s;
    }

    if (a.is_null()) {
        return "null type"s;
    }

    if (a.is_status()) {
        return "status type"s;
    }

    if (a.is_string()) {
        return "string type"s;
    }
    return "error"s;
}

auto operator << (std::ostream& os, const any& a) -> std::ostream& {
    return os << to_string(a);
}

}   // end of namespace result
}   // end of namespace redis


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
            freeReplyObject((redisReply*)r);    // like any "good" C interface this is not const :(
// #ifndef WIN32   // this would trigger run time error about accessing a freed memory on windows!
//             memset(redisReply*)r, 0, sizeof(*r));
// #endif  // WIN32
        }
    }

    void release_dummy(redisReply*)
    {
    }
}   // end of local namespace

namespace details {

lowlevel_access::lowlevel_access(internals* from) : 
    handler(from, release_reply) {

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

#if 0
reply::reply()
{
}

reply::reply(redisReply* rr) : data(rr, release_reply)
{
}

reply::reply(redisReply* rr, differ_t) : data(rr, release_dummy)
{
}

reply::~reply()
{
}

reply& reply::operator = (redisReply* rr)
{
    data.reset(rr, release_reply);
    return *this;
}

const char* reply::answer() const
{
    if (valid() && (data->type == REDIS_REPLY_STATUS || data->type == REDIS_REPLY_ERROR ||
            data->type == REDIS_REPLY_STRING)) {
        return data->str;
    } else {
        return 0;
    }
}

std::size_t reply::elements() const
{
    return valid() && data->type == REDIS_REPLY_ARRAY ? data->elements : 0;
}

bool reply::empty_list() const
{
    return elements() == 0;
}

reply reply::operator[] (std::size_t at) const
{
    if (elements() > at && what() == ARRAY) {
        return reply(data->element[at], differ);
    } else {
        return reply();
    }
}

long long reply::get() const
{
    return valid() && data->type == REDIS_REPLY_INTEGER ? data->integer : LLONG_MAX;
}

reply::result_type reply::what() const
{
    if (valid()) {
        switch (data->type) {
        case REDIS_REPLY_STATUS:
            return STATUS;
        case REDIS_REPLY_INTEGER:
            return INTEGER;
        case REDIS_REPLY_ARRAY:
            return ARRAY;
        case REDIS_REPLY_ERROR:
            return ERROR_MSG;
        case REDIS_REPLY_STRING:
            return STRING;
        case REDIS_REPLY_NIL:
        default:
            return NULL_MESSAGE;
        }
    } else {
        return NULL_MESSAGE;
    }
}

std::size_t reply::size() const
{
    return what() == STATUS || what() == ERROR_MSG || what() == STRING ? data->len : 0;
}

redisReply* reply::get_ptr()
{
    return data.get();
}

const redisReply* reply::get_ptr() const
{
    return data.get();
}


reply::operator boolean_type () const
{
    return what() != ERROR_MSG ? &reply::dummy : (boolean_type)0;
}

bool reply::valid() const
{
    return data.get() != 0;
}

std::ostream& operator << (std::ostream& os, const reply& rep)
{
    if (rep) {
        switch (rep.what()) {
        case reply::ARRAY:
            return os<<"array with "<<rep.elements()<<" elements";
        case reply::STRING:
            return os<<"'"<<rep.answer()<<"'";
        case reply::ERROR_MSG:
            return os<<"error: "<<rep.answer();
        case reply::INTEGER:
            return os<<rep.get();
        case reply::STATUS:
            return os<<"status: "<<rep.answer();
        default:
            return os<<"unknwon message type or empty reply";
        }
    } else {
        return os<<"invalid reply";
    }
}
#endif
}   // end of namespace result
}   // end of namespace redis


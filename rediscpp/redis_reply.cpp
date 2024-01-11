#include "redis_reply.h"
#include <hiredis/hiredis.h>
#include <limits.h>
#include <string.h>
#include <iostream>

namespace redis
{

namespace
{
    void release_reply(redisReply* r)
    {
        if (r && (r->type != 0 || r->integer != 0 || r->len != 0)) {
            freeReplyObject(r);
#ifndef WIN32   // this would trigger run time error about accessing a freed memory on windows!
            memset(r, 0, sizeof(*r));
#endif  // WIN32
        }
    }

    void replease_dummy(redisReply*)
    {
    }
}   // end of local namespace

reply::reply()
{
}

reply::reply(redisReply* rr) : data(rr, release_reply)
{
}

reply::reply(redisReply* rr, differ_t) : data(rr, replease_dummy)
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

}   // end of namespace redis


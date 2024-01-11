#pragma once

#include <string>
#include <boost/shared_ptr.hpp>
#include <iosfwd>


struct redisReply;

namespace redis
{
    static const struct differ_t {} differ = differ_t();

    // this is used for all message passing to the server and from the server
    // note that we need to make sure that we got a reply throug oeprator boolean i.e. if (reply) {
    // otherwise it would failed for every function here
    // this would be used for reading any type of message returned from the server.
    // normaly we would not use this explicitly unless we have to get this from the 
    // return value of an iterator
    struct reply
    {
        enum result_type
        {
            INTEGER, NULL_MESSAGE, STRING, ARRAY, STATUS, ERROR_MSG
        };

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
    };

    std::ostream& operator << (std::ostream& os, const reply& rep);
}   // end of namespace redis


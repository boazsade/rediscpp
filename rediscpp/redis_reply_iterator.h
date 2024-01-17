#pragma once


#include "redis_reply.h"
#include <boost/iterator/iterator_facade.hpp>
#include <optional>

namespace redis
{
    // use this to iterate over the messages got from the server in case this is 
    // array reply, otherwise this would not work..
    // the usage for this would work with redis array and multi maps and the interface and usage 
    // is the same as normal stl iterators
    struct reply_iterator : public boost::iterator_facade<reply_iterator, result::array, 
                                                          boost::random_access_traversal_tag,
                                                          result::any
                            >
    {
        explicit reply_iterator(result::array&& r);
        reply_iterator();

        friend difference_type at(const reply_iterator& i) {
            return i.index;
        }

    private:
        friend class boost::iterator_core_access;
        void increment();

        void decrement();

        void advance(difference_type n);   // move forward n steps

        bool equal(reply_iterator const& other) const;

        result::any dereference() const;
    private:
        std::optional<result::array> current;
        difference_type   index{0};
    };
}   // end of redis namespace


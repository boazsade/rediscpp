#include "redis_reply_iterator.h"
#include <limits.h>

namespace redis
{

namespace
{
    reply dummy = reply();
    const int invalid_index = -1;
}   // end of local namespace

reply_iterator::reply_iterator(const reply* r) : current(r && r->what() == reply::ARRAY ? *r : dummy), 
                                           index(r && r->what() == reply::ARRAY ? 0 : invalid_index)
{
}

void reply_iterator::increment()
{
    // make sure this is valid
    if (index != invalid_index  && index+1 < (int)current.elements()) {
        ++index;
    } else {    // reset it
        index = invalid_index;
    }
}

void reply_iterator::decrement()
{
    if (index != invalid_index  && index >= 1) {
        --index;
    }
}

void reply_iterator::advance(difference_type n)
{
    if (index != invalid_index) {       // sanity
        if (n < 0 && index + n >= 0) {  // go back
            index += n;
        } else {                        // go fardword
            if ((index + n) < (int)current.elements()) {
                index += n;
            }
        }
    }
}

bool reply_iterator::equal(reply_iterator const& other) const
{
    return ((cast(current) == cast(dummy)) && cast(other.current) == cast(dummy)) || (other.index == index);
}

reply reply_iterator::dereference() const
{
    if (index != invalid_index) {
        return current[index];
    } else {
        static const reply error = reply();
        return error;
    }
}

}   // end of namespace redis


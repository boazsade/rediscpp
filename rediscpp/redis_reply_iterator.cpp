#include "redis_reply_iterator.h"
#include <limits.h>

namespace redis
{

namespace
{
    //reply dummy = reply();
    const int invalid_index = -1;
}   // end of local namespace

reply_iterator::reply_iterator(result::array&& r) : current{std::move(r)} {
}

reply_iterator::reply_iterator() : current{std::nullopt}, index{invalid_index} {

}

void reply_iterator::increment()
{
    if (!current || index == invalid_index) {
        return;
    }
    // make sure this is valid
    if (index + 1 < (int)current->size()) {
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
    if (index != invalid_index && current) {       // sanity
        if (n < 0 && index + n >= 0) {  // go back
            index += n;
        } else {                        // go foreword
            if ((index + n) < (int)current->size()) {
                index += n;
            }
        }
    }
}

bool reply_iterator::equal(reply_iterator const& other) const
{
    //return ((cast(current) == cast(dummy)) && cast(other.current) == cast(dummy)) || (other.index == index);
    return index == other.index;
}

result::any reply_iterator::dereference() const
{
    if (index != invalid_index) {
        return current.value()[index];
    } else {
        static const result::any error {};
        return error;
    }
}

}   // end of namespace redis


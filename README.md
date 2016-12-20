# rediscpp
This is an attept to impelement the REDIS in memory database to be used from within C++ application where the REDIS API is not implemented in term of REDIS commands but rather in C++ STL like API. 
We have 
rstring - which is STL string like
rmap - which is STL map like 
long_int - which is long int like (in this case the POD long int and nothing in the STL itself)
rarray - which is STL vector like

Note that for all those data types we don't store the data inside them, they are proxies to the actual storage that take place inside REDIS.
What we really have is a connection to the REDIS inside any of those and when accessing the data we are either reading or changing data in the REDIS DB.
For the connection we have a class called endpoint which takes care of the networking issues (connecting to the REDIS database)
Another concept here is the subscriber/publisher model -  this implements in the channel concept - 
This is the subscriber/publisher pattern found in REDIS. We can create a channel the then subscribe or publish on this channel.
The last concept iterator - we can iterate on rarray data type - and it can be used with any of the STL algorithms since this iterator is fully compliante with STL iterators
All the code is under namespace REDIS, and for the first version it is C++98 compliante as well as VS and GCC compiled and tested for Windows and GCC (4.8). 
Later version would support C++14 on GCC 6 and later.
This relay on having your version of REDIS up and running as well as hiredis C found at https://github.com/redis/hiredis
For later versions I would remove the use of hiredis because of issues related to networking (and move to using ASIO which would soon be replaced with the up comming networking standard in C++).

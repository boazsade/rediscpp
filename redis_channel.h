#pragma once

#include "redis_endpoint.h"
#include <string>
#include <utility>

namespace redis
{
   /* usage:
        end_point connection(..);
        channel messages("my channel", connection);
        // in one thread or process do ..
        publisher sender = messages.make_publisher();
        sender.send("a message");
        // in another thread or process do 
        subscriber reader messages.make_subscriber();
        // do wait for more than 10 seconds
        message_type msg = reader.read(end_point::seconds(10));
        if (msg.second == subscriber::OK) {
            std::cout<<"got message "<<msg.first<<" from publisher\n";
        } else {
            std::cout<<"fail to get a message from the publisher in 10 seconds"<<std::endl;
        }
   */
    struct publisher;
    struct subscriber;

    struct channel
    {
        channel();  // default ctor dose nothing

        channel(const std::string& n, end_point srv);

        bool create(const std::string& n, end_point srv);

        const char* name() const;

        end_point& by() const;

        publisher make_publisher() const;

        subscriber make_subscriber() const;

        void close();

    private:
        mutable end_point server;
        std::string topic;
    };

    struct publisher
    {
        friend struct channel;
    private:
        publisher(const channel& c);

    public:
        ~publisher();

        void send(const std::string& msg) const;

    private:
        channel comm;
    };

    struct subscriber
    {
        friend struct channel;
#if defined(ERROR)
#        pragma push_macro("ERROR")
#        undef ERROR
#       define HAS_ERROR_MACRO_THAT_WAS_MASKED_SO_IGNORE
#endif  // ERROR
        enum state {
            ERROR = -1,
            TIME_OUT,
            DONE,
            OK
        };
#if defined(HAS_ERROR_MACRO_THAT_WAS_MASKED_SO_IGNORE)        
#   pragma pop_macro("ERROR")
#   undef HAS_ERROR_MACRO_THAT_WAS_MASKED_SO_IGNORE
#endif // HAS_ERROR_MACRO_THAT_WAS_MASKED_SO_IGNORE
        typedef std::pair<std::string, state> message_type;

    private:
        subscriber(const channel& c);

    public:
        ~subscriber();

        message_type read() const;

        message_type read(const end_point::timeout& to) const;

        message_type read(const end_point::seconds& s) const;

        message_type read(const end_point::milliseconds& ms) const;
        // would only work if this is running in different thread to finish this - once this is called
        // you would not be able to read from this channel any more!
        void interrupt() const;   

        void close() const;
    private:

        mutable channel comm;
        mutable bool    done;
    };
}


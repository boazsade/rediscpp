include(flags)
include(dependencies)

add_library(rediscpp STATIC           
           redis_channel.h  redis_channel.cpp 
           redis_endpoint.h redis_endpoint.cpp
           redis_messages.h redis_messages.cpp
           redis_multimap.h redis_multimap.cpp
           redis_reply.h redis_reply.cpp
           redis_reply_iterator.h redis_reply_iterator.cpp
	    ) 

target_include_directories(rediscpp PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>/../)
list(APPEND EXTRA_INCLUDES rediscpp)

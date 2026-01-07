cmake_minimum_required(VERSION 3.10)
project(epollComm)

#[[set(GOOGLETEST_VERSION 1.12.1)]]

set(CMAKE_CXX_STANDARD 14)

SET(CMAKE_CXX_FLAGS -lpthread)

find_package(Threads)

include_directories(.)
include_directories(../)

SET(SRC_EPOLL_COMM
  CIPSOCKET.cpp 
  CUDP.cpp
  CTCP.cpp 
  CBCast.cpp 
  CNetCommEpoll.cpp 
  ChannelManager.cpp
  EpollCommApi.cpp 
)

add_library(NetEpollComm ${SRC_EPOLL_COMM} )
target_link_libraries(NetEpollComm infoHub pthread)

include_directories(SYSTEM "/foo/bar")

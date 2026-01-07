cmake_minimum_required(VERSION 3.10)
project(infoHub)


set(CMAKE_CXX_STANDARD 14)

SET(CMAKE_CXX_FLAGS -lpthread)

find_package(Threads)

include_directories(.)
include_directories(../)

SET(SRC_INFO_HUB
    ./infoHub.cpp
    ./infoHub_impl.cpp
)

add_library(infoHub ${SRC_INFO_HUB} )
target_link_libraries(infoHub pthread)

include_directories(SYSTEM "/foo/bar")

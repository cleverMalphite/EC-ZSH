cmake_minimum_required(VERSION 3.10)
project(NetCombTransfer)

find_package(Threads)

# add_compile_options(-fsanitize=address -g)
SET(CMAKE_CXX_FLAGS -pthread)

set(CMAKE_CXX_STANDARD 14)

SET(CMAKE_CXX_FLAGS -lpthread)

include_directories(.)

SET(SRC_NETCOMBTRANSFER
  CNetBuffer.cpp 
  CIMsgChannelStatusNotify.cpp
  CNetChannel.cpp 
  CNetTerminal.cpp 
  DataCodec.cpp 
  CNetTerminalManager.cpp 
  NetCombTransferCmdThread.cpp 
  NetCombTransferApi.cpp 
  ../Util/SystemTimeFunc.cpp 
)

add_library(NetCombTransferAPI ${SRC_NETCOMBTRANSFER})

include_directories(SYSTEM "/foo/bar")
target_link_libraries(NetCombTransferAPI pthread ${LIBRARIES})


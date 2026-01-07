cmake_minimum_required(VERSION 3.10)
project(BulkDataTransfer)

set(CMAKE_CXX_STANDARD 14)

include_directories(${PROJECT_SOURCE_DIR})

# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

SET(SRC_BulkDataTransfer
    #./main_snd.cxx
)

#add_library(DataTransfer ${SRC_DATATRANSFER} )
#target_link_libraries(DataTransfer MRUDP IniHandler )



#封装MRUDP文件发送框架和文件接收框架
add_executable(main_bulksender
    ./main_bulksender.cpp
    ../DataTransfer/fileblock/msg_cpp/FileBlockMsg.pb.cc
    ../public/_public.cpp
)
target_link_libraries(main_bulksender
    IniHandler
    NetEpollComm
    NetCombTransferAPI
    SpeedControl
    MRUDP
    infoHub
    rpc
    hv
    ${CMAKE_THREAD_LIBS_INIT}  
)
target_link_directories(main_bulksender
    PRIVATE
    /usr/local/lib/
    ${LIB_DIRS}
)
target_include_directories(main_bulksender
    PRIVATE
    /usr/local/include/
    ${INC_DIRS}
)

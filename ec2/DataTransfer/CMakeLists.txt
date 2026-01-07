cmake_minimum_required(VERSION 3.10)
project(DataTransfer)

set(CMAKE_CXX_STANDARD 14)

include_directories(${PROJECT_SOURCE_DIR})

# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

SET(SRC_DATATRANSFER
    #./main_snd.cxx
)

#add_library(DataTransfer ${SRC_DATATRANSFER} )
#target_link_libraries(DataTransfer MRUDP IniHandler )


# 测试使用fileblock模块+mrudp模块进行文件发送和文件接收
add_executable(main_snd
    ./main_snd.cxx
    ./fileblock/msg_cpp/FileBlockMsg.pb.cc
    ../public/_public.cpp
)
target_link_libraries(main_snd
    IniHandler
    NetEpollComm
    NetCombTransferAPI
    SpeedControl
    MRUDP
    ${CMAKE_THREAD_LIBS_INIT}  
)
target_link_directories(main_snd
    PRIVATE
    ${LIB_DIRS}
)
target_include_directories(main_snd
    PRIVATE
    ${INC_DIRS}
)

add_executable(main_rcv
    ./main_rcv.cxx
    ./fileblock/msg_cpp/FileBlockMsg.pb.cc
    ../public/_public.cpp
)
target_link_libraries(main_rcv
    IniHandler
    NetEpollComm
    NetCombTransferAPI
    SpeedControl
    MRUDP
    ${CMAKE_THREAD_LIBS_INIT}  
)
target_link_directories(main_rcv
    PRIVATE
    ${LIB_DIRS}
)
target_include_directories(main_rcv
    PRIVATE
    ${INC_DIRS}
)

#封装MRUDP文件发送框架和文件接收框架
add_executable(main_sender
    ./main_sender.cxx
    ./fileblock/msg_cpp/FileBlockMsg.pb.cc
    ../public/_public.cpp
)
target_link_libraries(main_sender
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
target_link_directories(main_sender
    PRIVATE
    /usr/local/lib/
    ${LIB_DIRS}
)
target_include_directories(main_sender
    PRIVATE
    /usr/local/include/
    ${INC_DIRS}
)

add_executable(main_receiver
    ./main_receiver.cxx
    ./fileblock/msg_cpp/FileBlockMsg.pb.cc
    ../public/_public.cpp
)
target_link_libraries(main_receiver
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
target_link_directories(main_receiver
    PRIVATE
    /usr/local/lib/
    ${LIB_DIRS}
)
target_include_directories(main_receiver
    PRIVATE
    /usr/local/include/
    ${INC_DIRS}
)

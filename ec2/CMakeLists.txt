cmake_minimum_required(VERSION 3.10)
project(EmergencyCommunication)

# 应急系统

SET(CMAKE_CXX_FLAGS -pthread)

set(CMAKE_CXX_STANDARD 14)

set(CMAKE_BUILD_TYPE Debug)

set(INC_DIRS /usr/local/include )
set(LIB_DIRS /usr/local/lib)


#打開Qt的MOC功能和UI文件處理功能
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOMOC ON)

find_package(OpenCV REQUIRED)
find_package(Threads REQUIRED)
find_package(Qt5 COMPONENTS Core Gui Widgets Charts REQUIRED)
find_package(Protobuf REQUIRED)
include_directories(${PROTOBUF_INCLUDE_DIRS})

include_directories(SYSTEM "/foo/bar")

#指定头文件搜索路径
include_directories(${PROJECT_SOURCE_DIR})
include_directories(${PROJECT_SOURCE_DIR}/Util)
include_directories(${PROJECT_SOURCE_DIR}/public)

include_directories(${PROJECT_SOURCE_DIR}/infoHub)

include_directories(${PROJECT_SOURCE_DIR}/epollComm)
include_directories(${PROJECT_SOURCE_DIR}/IniHandle)
include_directories(${PROJECT_SOURCE_DIR}/NetCombTransfer)
include_directories(${PROJECT_SOURCE_DIR}/SpeedControl)
include_directories(${PROJECT_SOURCE_DIR}/MRUDP)
include_directories(${PROJECT_SOURCE_DIR}/RBUDP)
include_directories(${PROJECT_SOURCE_DIR}/BigDataTransfer)
include_directories(${PROJECT_SOURCE_DIR}/Term2TermQos)
include_directories(${PROJECT_SOURCE_DIR}/DataTransfer)
include_directories(${PROJECT_SOURCE_DIR}/BulkDataTransfer)

add_subdirectory(${PROJECT_SOURCE_DIR}/infoHub)

add_subdirectory(${PROJECT_SOURCE_DIR}/epollComm)
add_subdirectory(${PROJECT_SOURCE_DIR}/IniHandle)
add_subdirectory(${PROJECT_SOURCE_DIR}/NetCombTransfer)
add_subdirectory(${PROJECT_SOURCE_DIR}/SpeedControl)
add_subdirectory(${PROJECT_SOURCE_DIR}/MRUDP)
add_subdirectory(${PROJECT_SOURCE_DIR}/RBUDP)
add_subdirectory(${PROJECT_SOURCE_DIR}/BigDataTransfer)
add_subdirectory(${PROJECT_SOURCE_DIR}/Term2TermQos)
add_subdirectory(${PROJECT_SOURCE_DIR}/DataTransfer)
add_subdirectory(${PROJECT_SOURCE_DIR}/BulkDataTransfer)

set(LIBRARIES
  pthread
)


add_executable(ec_xgz_ui
        public/_public.cpp
        Util/LockGuard.h
        Util/DataUpdater.h
        ec2_ui_temp_cmake/main.cpp
        ec2_ui_temp_cmake/function.cpp
        ec2_ui_temp_cmake/mainwindow.cpp
        ec2_ui_temp_cmake/temppage.cpp
#        ec2_ui_temp_cmake/VideoReceiver.cpp
#        ec2_ui_temp_cmake/VideoReceiver.h
        ec2_ui_temp_cmake/VideoSender.cpp
        ec2_ui_temp_cmake/VideoSender.h
        ec2_ui_temp_cmake/playerwidget.h
        ec2_ui_temp_cmake/playerwidget.cpp

        DataTransfer/fileblock/msg_cpp/FileBlockMsg.pb.cc
#        DataTransfer/FileSender.h
#        DataTransfer/FileReceiver.h
#        public/utility.h
#        public/_public.h
#        DataTransfer/fileblock/FileBlock.h
#        DataTransfer/fileblock/ReadFileBlock.h
#        DataTransfer/fileblock/WriteFileBlock.h
#        DataTransfer/fileblock/msg_cpp/FileBlockMsg.pb.h


)
target_link_libraries(ec_xgz_ui
        IniHandler
        NetEpollComm
        NetCombTransferAPI
        SpeedControl
        MRUDP
        RBUDP
        BigDataTransfer
        QOS
        Qt5::Widgets
        Qt5::Charts
        infoHub
        rpc
        hv
        ${OpenCV_LIBS}
        ${CMAKE_THREAD_LIBS_INIT}
)
target_link_directories(ec_xgz_ui
        PRIVATE
        /usr/local/lib/
        ${LIB_DIRS}
)
target_include_directories(ec_xgz_ui
        PRIVATE
        /usr/local/include/
        ${INC_DIRS}
)


add_executable(ec_singletransfer 
    main_singletransfer.cpp
    public/_public.cpp
    Util/LockGuard.h
)
target_link_libraries(ec_singletransfer
    IniHandler
    NetEpollComm
    NetCombTransferAPI
    SpeedControl
    MRUDP
    RBUDP
    BigDataTransfer
    QOS
    rpc
)
add_executable(ec_singletransfer_2direction
    main_singletransfer_2direction.cpp
    public/_public.cpp
    Util/LockGuard.h
)
target_link_libraries(ec_singletransfer_2direction
    IniHandler
    NetEpollComm
    NetCombTransferAPI
    SpeedControl
    MRUDP
    RBUDP
    BigDataTransfer
    QOS
)

add_executable(ec_bulktransfer
    main_bulktransfer.cpp
    public/_public.cpp
    Util/LockGuard.h
)

target_link_libraries(ec_bulktransfer
    IniHandler
    NetEpollComm
    NetCombTransferAPI
    SpeedControl
    MRUDP
    RBUDP
    BigDataTransfer
    QOS
)


add_executable(main_mrudp1_1server
    main_mrudp1_1server.cpp
    public/_public.cpp
)
target_link_libraries(main_mrudp1_1server
    IniHandler
    NetEpollComm
    NetCombTransferAPI
    SpeedControl
    MRUDP
    RBUDP
    BigDataTransfer
    QOS
    infoHub
    rpc
    ${CMAKE_THREAD_LIBS_INIT}  
)
target_link_directories(main_mrudp1_1server
    PRIVATE
    ${LIB_DIRS}
)
target_include_directories(main_mrudp1_1server
    PRIVATE
    ${INC_DIRS}
)

add_executable(main_mrudp1_1server_2direction
    main_mrudp1_1server_2direction.cpp
    public/_public.cpp
)
target_link_libraries(main_mrudp1_1server_2direction
    IniHandler
    NetEpollComm
    NetCombTransferAPI
    SpeedControl
    MRUDP
    RBUDP
    BigDataTransfer
    QOS
    infoHub
    rpc
    ${CMAKE_THREAD_LIBS_INIT}  
)
target_link_directories(main_mrudp1_1server_2direction
    PRIVATE
    ${LIB_DIRS}
)
target_include_directories(main_mrudp1_1server_2direction
    PRIVATE
    ${INC_DIRS}
)


add_executable(main_mrudp1_1server_rpcclient
    main_mrudp1_1server_rpcclient.cpp
    public/_public.cpp
)
target_link_libraries(main_mrudp1_1server_rpcclient
    IniHandler
    NetEpollComm
    NetCombTransferAPI
    SpeedControl
    MRUDP
    RBUDP
    BigDataTransfer
    QOS
    infoHub
    rpc
    ${CMAKE_THREAD_LIBS_INIT}  
)
target_link_directories(main_mrudp1_1server_rpcclient
    PRIVATE
    ${LIB_DIRS}
)
target_include_directories(main_mrudp1_1server_rpcclient
    PRIVATE
    ${INC_DIRS}
)


add_executable(ec_bulktransfer_mrudp
    main_bulktransfer_mrudp.cpp
    public/_public.cpp
)
target_link_libraries(ec_bulktransfer_mrudp
    IniHandler
    NetEpollComm
    NetCombTransferAPI
    SpeedControl
    MRUDP
    RBUDP
    BigDataTransfer
    QOS
    infoHub
    ${CMAKE_THREAD_LIBS_INIT}  
)
target_link_directories(ec_bulktransfer_mrudp
    PRIVATE
    ${LIB_DIRS}
)
target_include_directories(ec_bulktransfer_mrudp
    PRIVATE
    ${INC_DIRS}
)


add_executable(main_bulktransfer_mrudp_thread
        main_bulktransfer_mrudp_thread.cpp
        public/_public.cpp
)
target_link_libraries(main_bulktransfer_mrudp_thread
        IniHandler
        NetEpollComm
        NetCombTransferAPI
        SpeedControl
        MRUDP
        RBUDP
        BigDataTransfer
        QOS
        infoHub
        ${CMAKE_THREAD_LIBS_INIT}
)
target_link_directories(main_bulktransfer_mrudp_thread
        PRIVATE
        ${LIB_DIRS}
)
target_include_directories(main_bulktransfer_mrudp_thread
        PRIVATE
        ${INC_DIRS}
)

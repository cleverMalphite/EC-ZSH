cmake_minimum_required(VERSION 3.10)
project(ec2_ui_temp_cmake)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)
#打開Qt的MOC功能和UI文件處理功能
set(CMAKE_AUTOMOC ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin)
# 指定头文件和库的路径
set(INC_DIRS /usr/local/include)
set(LIB_DIRS /usr/local/lib)

# 查找库
find_package(Threads REQUIRED)

find_package(Qt5 REQUIRED COMPONENTS Core Gui Widgets Charts)
find_package(OpenCV REQUIRED)
find_package(Protobuf REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(GST REQUIRED gstreamer-1.0 gstreamer-app-1.0)

include_directories(${INC_DIRS})
include_directories(${LIB_DIRS})

include_directories(${Protobuf_INCLUDE_DIRS})

set(PROJECT_DIR ../.)

# 指定头文件搜索路径
include_directories(${PROJECT_DIR}/.)
include_directories(${PROJECT_DIR}/epollComm)
include_directories(${PROJECT_DIR}/IniHandle)
include_directories(${PROJECT_DIR}/NetCombTransfer)
include_directories(${PROJECT_DIR}/SpeedControl)
include_directories(${PROJECT_DIR}/MRUDP)
include_directories(${PROJECT_DIR}/RBUDP)
include_directories(${PROJECT_DIR}/BigDataTransfer)
include_directories(${PROJECT_DIR}/Term2TermQos)

include_directories(${PROJECT_DIR}/Util)
include_directories(${PROJECT_DIR}/public)

set(LIBRARIES
        pthread
)

SET(TEST_UI
        ${PROJECT_DIR}/build/IniHandle
        ${PROJECT_DIR}/build/epollComm
        ${PROJECT_DIR}/build/NetCombTransfer
        ${PROJECT_DIR}/build/SpeedControl
        ${PROJECT_DIR}/build/MRUDP
        ${PROJECT_DIR}/build/RBUDP
        ${PROJECT_DIR}/build/BigDataTransfer
        ${PROJECT_DIR}/build/infoHub
        ${PROJECT_DIR}/build/Term2TermQos
)

link_directories(${TEST_UI})

# 设置可执行文件
add_executable(ec2_ui_test
        main.cpp
        ../public/_public.cpp
        function.cpp
        mainwindow.cpp
        temppage.cpp
        VideoSender.cpp
        VideoReceiver.cpp
        AudioSenderCore.cpp
        AudioReceiverCore.cpp
        playerwidget.cpp
        ${PROJECT_DIR}/DataTransfer/fileblock/msg_cpp/FileBlockMsg.pb.cc
        ${PROJECT_DIR}/Util/DataUpdater.h
#        ../epollComm/EpollCommApi.cpp  # 确保包含
#        ../utilClass/DataUpdater.h
#        utilClass/isRBUDP_callback.cpp
#        utilClass/isRBUDP_callback.h
)

add_executable(ec2_audio_selftest
        audio_selftest.cpp
        AudioReceiverCore.cpp
)

target_link_libraries(ec2_audio_selftest
        Qt5::Core
        Threads::Threads
        ${GST_LIBRARIES}
)

target_link_directories(ec2_audio_selftest
        PRIVATE
        /usr/local/lib/
        ${LIB_DIRS}
)

target_include_directories(ec2_audio_selftest PRIVATE
        ${GST_INCLUDE_DIRS}
)

target_compile_options(ec2_audio_selftest PRIVATE
        ${GST_CFLAGS_OTHER}
)

# 链接库
target_link_libraries(ec2_ui_test
        Qt5::Core
        Qt5::Gui
        Qt5::Widgets
        Qt5::Charts
        Threads::Threads
        ${OpenCV_LIBS}
        ${GST_LIBRARIES}
        ${Protobuf_LIBRARIES}
        IniHandler
        infoHub
        NetEpollComm
        NetCombTransferAPI
        SpeedControl
        MRUDP
        RBUDP
        BigDataTransfer
        QOS
        rpc
        hv
)

target_link_directories(ec2_ui_test
        PRIVATE
        /usr/local/lib/
        ${LIB_DIRS}
)

target_include_directories(ec2_ui_test PRIVATE
        ${GST_INCLUDE_DIRS}
)

target_compile_options(ec2_ui_test PRIVATE
        ${GST_CFLAGS_OTHER}
)

# 设置 CMake 策略
if(COMMAND cmake_policy)
    cmake_policy(SET CMP0003 NEW)
    cmake_policy(SET CMP0015 NEW)
    cmake_policy(SET CMP0081 NEW)
endif()

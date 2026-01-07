cmake_minimum_required(VERSION 3.10)
project(TEST_MRUDP)

# 应急系统

set(CMAKE_CXX_FLAGS -pthread)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_BUILD_TYPE Debug)


find_package(Protobuf REQUIRED)
include_directories(${PROTOBUF_INCLUDE_DIRS})

set(PROJECT_DIR ../.)

#指定头文件搜索路径
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

include_directories(../../msg_cpp)

set(LIBRARIES
  pthread
)

SET(TEST_MRUDP_LIB_DIR
  ${PROJECT_DIR}/build/IniHandle
  ${PROJECT_DIR}/build/epollComm
  ${PROJECT_DIR}/build/NetCombTransfer
  ${PROJECT_DIR}/build/SpeedControl
  ${PROJECT_DIR}/build/MRUDP
  ${PROJECT_DIR}/build/RBUDP
  ${PROJECT_DIR}/build/BigDataTransfer
  ${PROJECT_DIR}/build/Term2TermQos
)

link_directories(${TEST_MRUDP_LIB_DIR})

#=========== TEST MRUDP DATA_SERVER_CALLBACK ==========
#----------------test mrudp 1 : 1server ----------------
add_executable(test_mrudp1_1server 
    test_mrudp1_1server.cpp
    ${PROJECT_DIR}/public/_public.cpp
)
target_link_libraries(test_mrudp1_1server
  IniHandler
  NetEpollComm
  NetCombTransferAPI
  SpeedControl
  MRUDP
  ${PROTOBUF_LIBRARIES}
)

#----------------test mrudp 2 : 2server ----------------
add_executable(test_mrudp2_2server 
    test_mrudp2_2server.cpp
    ${PROJECT_DIR}/public/_public.cpp
)
target_link_libraries(test_mrudp2_2server
  IniHandler
  NetEpollComm
  NetCombTransferAPI
  SpeedControl
  MRUDP
  ${PROTOBUF_LIBRARIES}
)

#----------test mrudp 3 : file transfer with msg--------
add_executable(test_mrudp3_filetrans_withmsg
  test_mrudp3_filetrans_withmsg.cpp
  ${PROJECT_DIR}/public/_public.cpp
)
target_link_libraries(test_mrudp3_filetrans_withmsg
  IniHandler
  NetEpollComm
  NetCombTransferAPI
  SpeedControl
  MRUDP
  RBUDP
  BigDataTransfer
  QOS
  ${PROTOBUF_LIBRARIES}
)

#=========== TEST RBUDP DATA_SERVER_CALLBACK ==========
#----------test rbudp 1 : file transfer not with msg--------
add_executable(test_rbudp1_filetrans
  test_rbudp1_filetrans.cpp
  ${PROJECT_DIR}/public/_public.cpp
)
target_link_libraries(test_rbudp1_filetrans
  IniHandler
  NetEpollComm
  NetCombTransferAPI
  SpeedControl
  MRUDP
  RBUDP
  BigDataTransfer
  QOS
  ${PROTOBUF_LIBRARIES}
)

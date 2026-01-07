cmake_minimum_required(VERSION 3.10)
project(MRUDP)

set(CMAKE_CXX_STANDARD 14)


#----------protobuf配置----------------
#添加protobuf配置
find_package(Protobuf REQUIRED)

# 包含Protobuf头文件
include_directories(${PROTOBUF_INCLUDE_DIRS})

# 生成Protobuf消息
protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ../msg_proto)


include_directories(${PROJECT_SOURCE_DIR}/)

SET(SRC_MRUDP
  ../msg_cpp/msg.pb.cc
  ../Util/SystemTimeFunc.cpp
  Transfer_CPP/ReliableUdpData.cpp
  Transfer_CPP/ReliableUdpMessage.cpp
  AbstractStorage_CPP/FileStorage.cpp
  AbstractStorage_CPP/MemoryPartStorage.cpp
  CircularQueue_CPP/SendCircularDataQueue.cpp
  CircularQueue_CPP/RecvCircularDataQueue.cpp
  Transfer_CPP/ReliableForceSpeedMessage.cpp
  Transfer_CPP/ReliableForceAdjustBandWidthMessage.cpp
  ReliableTransferLog.cpp
  End2EndReliableTransmission.cpp
  SendWindowStatusRecord.cpp
  MRUDPSettingUtil.cpp
  SerializeUtil.cpp
  MRUDPManager.cpp
  MRUDPApi.cpp
)

add_library(MRUDP ${SRC_MRUDP} )

target_link_libraries(MRUDP 
    IniHandler
    NetCombTransferAPI
    SpeedControl
    ${PROTOBUF_LIBRARIES}
)

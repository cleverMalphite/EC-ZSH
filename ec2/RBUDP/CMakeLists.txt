cmake_minimum_required(VERSION 3.10)
project(RBUDP)

set(CMAKE_CXX_STANDARD 14)

include_directories(${PROJECT_SOURCE_DIR}/)

SET(SRC_RBUDP
  ../msg_cpp/msg.pb.cc
  ../Util/SystemTimeFunc.cpp
  SerializeUtil_RBUDP.cpp 
  CRBUDPTerm2TermConfig.cpp
  Transfer_CPP/RBUDPdata.cpp
  Transfer_CPP/NAK_Message.cpp 
  QueueMap_CPP/RecvDataQueueMap.cpp
  QueueMap_CPP/SendDataQueueMap.cpp 
  AbstractStorage_RBUDP_CPP/FileStorage.cpp
  End2EndReliableTransmission.cpp 
  RBUDPTempData.cpp
  RBUDPManager.cpp
  RBUDPApi.cpp 
)

add_library(RBUDP ${SRC_RBUDP})


target_link_libraries(RBUDP 
    IniHandler
    NetCombTransferAPI
    SpeedControl
)

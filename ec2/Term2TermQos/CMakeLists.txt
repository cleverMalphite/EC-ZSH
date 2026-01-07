cmake_minimum_required(VERSION 3.10)
project(QOS)

set(CMAKE_CXX_STANDARD 14)

include_directories(${PROJECT_SOURCE_DIR}/)

SET(SRC_TERM2TERM_QOS
  bbr_circlequeue.cpp 
  bbr_received_circle_packet.cpp
  bbr_sendhistory_circle_queue.cpp 
  bbr_skiplist.cpp DataCodec.cpp
  dllmain.cpp 
  EndToEndQos.cpp 
  Remote2SelfPacketQosFeedbackPacket.cpp
  Remote2SelfQosUnit.cpp 
  SelfToRemoteQosUnit.cpp 
  TermToTermQosApi.cpp
  TermToTermQosManager.cpp
)

SET(HEAD_TERM2TERM_QOS
  bbr_circlequeue.h 
  bbr_controller.h 
  bbr_received_circle_packet.h
  bbr_sendhistory_circle_queue.h 
  bbr_skiplist.h 
  DataCodec.h
  EndToEndQos.h 
  InterfaceDataAbstractBuffer.h 
  Remote2SelfPacketQosFeedbackPacket.h
  Remote2SelfQosUnit.h 
  SelfToRemoteQosUnit.h 
  TermToTermQosApi.h
  TermToTermQosManager.h
)

add_library(QOS ${SRC_TERM2TERM_QOS} ${HEAD_TERM2TERM_QOS})

target_link_libraries(QOS IniHandler  NetCombTransferAPI SpeedControl MRUDP RBUDP)

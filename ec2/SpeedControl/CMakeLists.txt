cmake_minimum_required(VERSION 3.10)
project(SpeedControl)

set(CMAKE_CXX_STANDARD 14)

include_directories(${PROJECT_SOURCE_DIR}/)

SET(SRC_SPEED_CONTROL
  SpeedControlApi.cpp
  DataWithPacketInfo.cpp 
  Term2TermTransmission.cpp
  SpeedControlManager.cpp 
  SpeedControlCallBackFunc.cpp
  ../Util/VideoRtp_callback.cpp
  ../Util/AudioRtp_callback.cpp
)

add_library(SpeedControl ${SRC_SPEED_CONTROL} )

target_link_libraries(SpeedControl IniHandler NetCombTransferAPI RBUDP infoHub)

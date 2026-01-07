cmake_minimum_required(VERSION 3.10)
project(BigDataTransfer)

set(CMAKE_CXX_STANDARD 14)

include_directories(${PROJECT_SOURCE_DIR})

# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

SET(SRC_BIGDATATRANSFER
  DataAndMessage_CPP/AbstractDataOrMessage.cpp
  DataAndMessage_CPP/FileTransferData.cpp
  DataAndMessage_CPP/FileRecvFailed.cpp
  DataAndMessage_CPP/FileRecvSuccess.cpp
  DataAndMessage_CPP/FileSendTaskCancel.cpp
  FileTransferTask_CPP/FileTransferTask.cpp
  FileTransferTask_CPP/FileTransferSendTask.cpp
  FileTransferTask_CPP/FileTransferRecvTask.cpp
  BigDataTransferManager.cpp
  BigDataTransferApi.cpp
)

add_library(BigDataTransfer ${SRC_BIGDATATRANSFER} )

target_link_libraries(BigDataTransfer MRUDP RBUDP IniHandler )

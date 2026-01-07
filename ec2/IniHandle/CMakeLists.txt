cmake_minimum_required(VERSION 3.10)

project(IniHandle)

set(CMAKE_CXX_STANDARD 14)

#add_executable(IniHandle INIReader.h IniHandleApi.h IniHandleApi.cpp)

add_library(IniHandler IniHandleApi.h IniHandleApi.cpp)

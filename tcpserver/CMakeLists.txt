cmake_minimum_required(VERSION 3.20)

# Set the project name and version
project(tcpserver VERSION 1.0)

# Specify the C++ standard
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(SOURCES
src/TCPServer.cpp
src/main.cpp
)

set(INCLUDES
include/Clientstate.hpp
include/ctpl_stl.h
include/TCPServer.hpp
)

add_executable(tcpserver ${SOURCES} ${INCLUDES})
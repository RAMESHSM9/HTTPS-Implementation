cmake_minimum_required(VERSION 3.2)
project(TCPClient)

set(SOURCES
src/TCPClient.cpp
)

add_executable(TCPClient ${SOURCES})
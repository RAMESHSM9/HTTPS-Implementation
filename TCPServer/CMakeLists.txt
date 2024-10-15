cmake_minimum_required(VERSION 3.2)
project(TCPServerDemo)

set(SOURCES
src/TCPServer.cpp
)

set(HEADERS
include/ctpl_stl.h
)

add_executable(TCPServerDemo ${SOURCES} ${HEADERS})

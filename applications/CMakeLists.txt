project(applications)

set(CMAKE_CXX_STANDARD 17)

set(SOURCES
src/tictactoe.cpp
)
add_executable(applications ${SOURCES})
target_link_libraries(applications httpserver)
set(INCLUDE_SERVER_DIRS "../httpserver")
target_include_directories(applications PUBLIC ${INCLUDE_SERVER_DIRS})

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/apps/tictactoe.html
     DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/apps/index.html
     DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
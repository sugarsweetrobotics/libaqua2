cmake_minimum_required(VERSION 2.8)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(aqua2_test)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

option(BUILD_SOCKET_TEST OFF)
option(BUILD_SERIALPORT_TEST "Build SerialPort class test" ON)

if(BUILD_SERIALPORT_TEST)
add_executable(serialport_test tests/serialport_test.cpp)
endif(BUILD_SERIALPORT_TEST)

if(${BUILD_SOCKET_TEST})
add_executable(sockettest sockettest.cpp)
if(WIN32)
  target_link_libraries(sockettest wsock32 ws2_32)
endif()
add_executable(serversockettest serversockettest.cpp)
if(WIN32)
  target_link_libraries(serversockettest wsock32 ws2_32)
endif()
endif(${BUILD_SOCKET_TEST})

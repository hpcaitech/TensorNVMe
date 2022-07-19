cmake_minimum_required(VERSION 3.16)
project(
        tensornvme
        DESCRIPTION "A tensor disk offloader without data copying."
        LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

add_subdirectory(3rd)
add_subdirectory(csrc)

if(EXISTS ${CMAKE_SOURCE_DIR}/tests)
        add_subdirectory(tests)
endif()
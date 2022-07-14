cmake_minimum_required(VERSION 3.16)
project(
        colo_nvme
        VERSION 0.1
        DESCRIPTION "ColossalAI NVME"
        LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)
#set(CMAKE_CXX_FLAGS                " ${CMAKE_CXX_FLAGS_INIT} -std=c++14 -fPIC")
#set(CMAKE_CXX_FLAGS_DEBUG          "-g -DDEBUG")
#set(CMAKE_CXX_FLAGS_MINSIZEREL     "-Os -DNDEBUG")
#set(CMAKE_CXX_FLAGS_RELEASE        "-O4 -DNDEBUG")
#set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2 -g -DDEBUG")

add_subdirectory(3rd)
add_subdirectory(csrc)

#if((CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME OR MODERN_CMAKE_BUILD_TESTING) AND BUILD_TESTING)
add_subdirectory(tests)
#endif()
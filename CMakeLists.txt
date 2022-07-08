cmake_minimum_required(VERSION 3.16)
project(nvme)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

include_directories(csrc/include)
include_directories(tests)

add_executable(test_asyncio tests/test_asyncio.cpp
        csrc/uring.cpp
        csrc/aio.cpp)
target_link_libraries(test_asyncio uring aio)

add_executable(test_space_mgr tests/test_space_mgr.cpp
        csrc/space_mgr.cpp)

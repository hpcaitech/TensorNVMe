cmake_minimum_required(VERSION 3.16)
project(nvme)

find_package(Torch REQUIRED
        PATHS $ENV{CMAKE_TORCH_PATH} # look here
        NO_DEFAULT_PATH)
find_library(TORCH_PYTHON_LIBRARY torch_python PATHS $ENV{CMAKE_TORCH_PATH}/lib)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${TORCH_CXX_FLAGS}")

include_directories($ENV{CMAKE_URING_PATH}/src/include)
link_directories($ENV{CMAKE_URING_PATH}/src)

include_directories($ENV{CMAKE_AIO_PATH}/src/)
link_directories($ENV{CMAKE_AIO_PATH}/src)

include_directories(csrc)
include_directories(csrc/include)

add_executable(test_asyncio tests/test_asyncio.cpp
        csrc/include/asyncio.h
        csrc/include/uring.h
        csrc/include/aio.h
        tests/catch.hpp
        csrc/uring.cpp
        csrc/aio.cpp)
target_link_libraries(test_asyncio "${TORCH_LIBRARIES}" uring aio ${TORCH_PYTHON_LIBRARY})

add_executable(test_space_mgr tests/test_space_mgr.cpp
        csrc/include/space_mgr.h
        csrc/include/asyncio.h
        csrc/include/uring.h
        csrc/include/aio.h
        csrc/uring.cpp
        csrc/aio.cpp
        tests/catch.hpp
        csrc/offload.cpp
        csrc/space_mgr.cpp)
target_link_libraries(test_space_mgr "${TORCH_LIBRARIES}" uring aio ${TORCH_PYTHON_LIBRARY})

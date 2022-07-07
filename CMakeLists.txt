cmake_minimum_required(VERSION 3.16)
project(nvme)

find_package(Torch REQUIRED
        PATHS $ENV{CMAKE_TORCH_PATH} # look here
        NO_DEFAULT_PATH)
find_library(TORCH_PYTHON_LIBRARY torch_python PATHS $ENV{CMAKE_TORCH_PATH}/lib)

find_library(URING PATHS $ENV{CMAKE_URING_PATH})

include_directories($ENV{CMAKE_AIO_PATH}/include)
link_directories($ENV{CMAKE_AIO_PATH}/lib)

include_directories($ENV{CMAKE_PYTHON_PATH}/include)
link_directories($ENV{CMAKE_PYTHON_PATH}/lib)

include_directories(csrc)
include_directories(csrc/include)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${TORCH_CXX_FLAGS}")

add_executable(test_asyncio tests/test_asyncio.cpp
        csrc/include/asyncio.h
        csrc/include/uring.h
        csrc/include/aio.h
        tests/catch.hpp
        csrc/uring.cpp
        csrc/aio.cpp)
target_link_libraries(test_asyncio "${TORCH_LIBRARIES}" uring aio python3 ${TORCH_PYTHON_LIBRARY})

add_executable(test_space_mgr tests/test_space_mgr.cpp
        csrc/include/space_mgr.h
        tests/catch.hpp
        csrc/space_mgr.cpp)
target_link_libraries(test_space_mgr "${TORCH_LIBRARIES}" python3 ${TORCH_PYTHON_LIBRARY})

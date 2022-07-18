cmake_minimum_required(VERSION 3.16)
project(
        colo_nvme
        VERSION 0.1
        DESCRIPTION "ColossalAI NVME"
        LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

add_subdirectory(3rd)
add_subdirectory(csrc)

#if((CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME OR MODERN_CMAKE_BUILD_TESTING) AND BUILD_TESTING)
add_subdirectory(tests)
#endif()
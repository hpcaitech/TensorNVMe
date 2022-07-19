cmake_minimum_required(VERSION 3.16)
project(
        ColossalAI-NVME
        VERSION 0.1
        DESCRIPTION "ColossalAI NVME"
        LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

add_subdirectory(3rd)
add_subdirectory(csrc)

if(EXISTS tests)
        add_subdirectory(tests)
endif()
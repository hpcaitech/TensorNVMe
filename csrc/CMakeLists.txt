add_library(colo_asyncio
        OBJECT
        aio.cpp
        uring.cpp)
target_link_libraries(colo_asyncio
        PUBLIC uring
        PUBLIC aio)
target_include_directories(colo_asyncio PUBLIC ../include)


add_library(space_mgr
        OBJECT
        space_mgr.cpp)
target_include_directories(space_mgr PUBLIC ../include)


include_directories(/home/lczxl/.local/packages/python3/include/python3.8)
link_directories(/home/lczxl/.local/packages/python3/lib/python3.8)
find_package(Torch REQUIRED NO_MODULE PATHS /home/lczxl/.local/packages/libtorch NO_DEFAULT_PATH)
add_library(offload OBJECT offload.cpp)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${TORCH_CXX_FLAGS}")
target_link_libraries(offload PUBLIC space_mgr PUBLIC colo_asyncio python3.8 "${TORCH_LIBRARIES}")
target_include_directories(offload PUBLIC ../include)

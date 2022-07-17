add_library(colo_asyncio
        SHARED
        aio.cpp
        uring.cpp)
target_link_libraries(colo_asyncio
        PUBLIC uring
        PUBLIC aio)
target_include_directories(colo_asyncio PUBLIC ../include)


add_library(space_mgr
        SHARED
        space_mgr.cpp)
target_include_directories(space_mgr PUBLIC ../include)



find_package(Torch REQUIRED NO_MODULE PATHS $ENV{CMAKE_TORCH_PATH} NO_DEFAULT_PATH)
find_library(TORCH_PYTHON_LIBRARY torch_python PATHS $ENV{CMAKE_TORCH_PATH}/lib)
add_library(offload SHARED offload.cpp)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${TORCH_CXX_FLAGS}")

# I can't compile without linking python
#include_directories(/home/lczxl/.local/packages/python3/include/python3.8)
#link_directories(/home/lczxl/.local/packages/python3/lib/python3.8)
#target_link_libraries(offload PUBLIC space_mgr PUBLIC colo_asyncio python3.8 "${TORCH_LIBRARIES}" ${TORCH_PYTHON_LIBRARY})

target_link_libraries(offload PUBLIC space_mgr PUBLIC colo_asyncio "${TORCH_LIBRARIES}" ${TORCH_PYTHON_LIBRARY})

target_include_directories(offload PUBLIC ../include)

pybind11_add_module(off_load MODULE offload.cpp)
target_link_libraries(off_load PRIVATE offload)

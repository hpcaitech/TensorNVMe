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


include_directories(/home/lczxl/.local/packages/python3/include/python3.8)
link_directories(/home/lczxl/.local/packages/python3/lib/python3.8)
find_package(Torch REQUIRED NO_MODULE PATHS /home/lczxl/.local/packages/libtorch NO_DEFAULT_PATH)
find_library(TORCH_PYTHON_LIBRARY torch_python PATHS /home/lczxl/.local/packages/libtorch/lib)
add_library(offload SHARED offload.cpp)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${TORCH_CXX_FLAGS}")
target_link_libraries(offload PUBLIC space_mgr PUBLIC colo_asyncio python3.8 "${TORCH_LIBRARIES}" ${TORCH_PYTHON_LIBRARY})
target_include_directories(offload PUBLIC ../include)

pybind11_add_module(off_load MODULE offload.cpp)
target_link_libraries(off_load PRIVATE offload)
#file(GLOB_RECURSE PY_PROJ_FILES ${CMAKE_SOURCE_DIR}/colo_nvme/*.p*)
#add_custom_command(OUTPUT ${CMAKE_SOURCE_DIR}/build/.timestamp
#        COMMAND ${PYTHON_EXECUTABLE} setup.py bdist_wheel
#        COMMAND ${CMAKE_COMMAND} -E touch ${CMAKE_SOURCE_DIR}/build/.timestamp
#        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
#        DEPENDS colo_nvme ${PY_PROJ_FILES})
#add_custom_target(colo_nvme_cp ALL DEPENDS ${CMAKE_SOURCE_DIR}/build/.timestamp)

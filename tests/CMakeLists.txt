add_executable(test_asyncio
        test_asyncio.cpp)
target_link_libraries(test_asyncio colo_asyncio)
target_include_directories(test_asyncio INTERFACE .)
add_test(NAME test_asyncio COMMAND test_asyncio)


add_executable(test_space_mgr
        test_space_mgr.cpp)
target_link_libraries(test_space_mgr space_mgr)
target_include_directories(test_space_mgr INTERFACE .)
add_test(NAME test_space_mgr COMMAND test_space_mgr)


find_package(Torch REQUIRED NO_MODULE PATHS /home/lczxl/.local/packages/libtorch NO_DEFAULT_PATH)
include_directories(/home/lczxl/.local/packages/python3/include/python3.8)
link_directories(/home/lczxl/.local/packages/python3/lib/python3.8)
add_executable(test_offload
        test_offload.cpp)
target_link_libraries(test_offload offload space_mgr colo_asyncio)
target_include_directories(test_offload INTERFACE .)
add_test(NAME test_offload COMMAND test_offload)

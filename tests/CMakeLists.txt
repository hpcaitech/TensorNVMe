add_executable(test_asyncio
        test_asyncio.cpp)
target_link_libraries(test_asyncio
        colo_asyncio)
target_include_directories(test_asyncio PUBLIC .)
add_test(NAME test_asyncio COMMAND test_asyncio)


add_executable(test_space_mgr
        test_space_mgr.cpp)
target_link_libraries(test_space_mgr space_mgr)
target_include_directories(test_space_mgr PUBLIC .)
add_test(NAME test_space_mgr COMMAND test_space_mgr)

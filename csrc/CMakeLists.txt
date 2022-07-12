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

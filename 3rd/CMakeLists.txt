include(ExternalProject)
find_library(LIBURING uring ${CMAKE_CURRENT_BINARY_DIR}/lib)

if(LIBURING)
    message("liburing is found in ${LIBURING}")
else(LIBURING)
    message("liburing is not found")
    ExternalProject_Add(extern_uring
        GIT_REPOSITORY https://github.com/axboe/liburing.git
        GIT_TAG liburing-2.2
        BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/liburing
        SOURCE_DIR ${CMAKE_CURRENT_BINARY_DIR}/liburing
        INSTALL_DIR ${CMAKE_CURRENT_BINARY_DIR}/liburing
        CONFIGURE_COMMAND ./configure --prefix=${CMAKE_CURRENT_BINARY_DIR}
        BUILD_COMMAND make && make install
    )

    add_library(uring STATIC IMPORTED GLOBAL)
    SET_PROPERTY(TARGET uring PROPERTY IMPORTED_LOCATION ${CMAKE_CURRENT_BINARY_DIR}/lib/liburing.a)
    file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/include/)
    target_include_directories(uring INTERFACE ${CMAKE_CURRENT_BINARY_DIR}/include/)
    add_dependencies(uring extern_uring)
endif(LIBURING)

find_library(LIBAIO aio ${CMAKE_CURRENT_BINARY_DIR}/lib)

if(LIBAIO)
    message("libaio is found in ${LIBAIO}")
else(LIBAIO)
    message("libaio is not found")
    ExternalProject_Add(extern_aio
        GIT_REPOSITORY https://pagure.io/libaio.git
        GIT_TAG libaio-0.3.113
        BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/libaio
        SOURCE_DIR ${CMAKE_CURRENT_BINARY_DIR}/libaio
        INSTALL_DIR ${CMAKE_CURRENT_BINARY_DIR}/libaio
        CONFIGURE_COMMAND ""
        UPDATE_COMMAND ""
        BUILD_COMMAND make prefix=${CMAKE_CURRENT_BINARY_DIR} install
        INSTALL_COMMAND ""
    )

    add_library(aio STATIC IMPORTED GLOBAL)
    SET_PROPERTY(TARGET aio PROPERTY IMPORTED_LOCATION ${CMAKE_CURRENT_BINARY_DIR}/lib/libaio.a)
    file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/include/)
    target_include_directories(aio INTERFACE ${CMAKE_CURRENT_BINARY_DIR}/include/)
    add_dependencies(aio extern_aio)
endif(LIBAIO)
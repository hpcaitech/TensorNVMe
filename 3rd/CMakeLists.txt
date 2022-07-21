include(ExternalProject)

if(DEFINED BACKEND_INSTALL_PREFIX)
    set(INSTALL_PREFIX ${BACKEND_INSTALL_PREFIX})
else()
    set(INSTALL_PREFIX ${CMAKE_CURRENT_BINARY_DIR})
endif()

if(DEFINED ENV{LD_LIBRARY_PATH})
    string(REPLACE ":" ";" CMAKE_LIBRARY_PATH $ENV{LD_LIBRARY_PATH})
endif()

find_library(LIBURING uring ${INSTALL_PREFIX}/lib)

if(LIBURING)
    message("liburing is found in ${LIBURING}")
else(LIBURING)
    message("liburing is not found, install in ${INSTALL_PREFIX}")
    ExternalProject_Add(extern_uring
        GIT_REPOSITORY https://github.com/axboe/liburing.git
        GIT_TAG liburing-2.2
        BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/liburing
        SOURCE_DIR ${CMAKE_CURRENT_BINARY_DIR}/liburing
        INSTALL_DIR ${CMAKE_CURRENT_BINARY_DIR}/liburing
        CONFIGURE_COMMAND ./configure --prefix=${INSTALL_PREFIX}
        BUILD_COMMAND make && make install
    )

    add_library(uring STATIC IMPORTED GLOBAL)
    SET_PROPERTY(TARGET uring PROPERTY IMPORTED_LOCATION ${INSTALL_PREFIX}/lib/liburing.a)
    file(MAKE_DIRECTORY ${INSTALL_PREFIX}/include/)
    target_include_directories(uring INTERFACE ${INSTALL_PREFIX}/include/)
    add_dependencies(uring extern_uring)
endif(LIBURING)

find_library(LIBAIO aio ${INSTALL_PREFIX}/lib)

if(LIBAIO)
    message("libaio is found in ${LIBAIO}")
else(LIBAIO)
    message("libaio is not found, install in ${INSTALL_PREFIX}")
    ExternalProject_Add(extern_aio
        GIT_REPOSITORY https://pagure.io/libaio.git
        GIT_TAG libaio-0.3.113
        BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/libaio
        SOURCE_DIR ${CMAKE_CURRENT_BINARY_DIR}/libaio
        INSTALL_DIR ${CMAKE_CURRENT_BINARY_DIR}/libaio
        CONFIGURE_COMMAND ""
        UPDATE_COMMAND ""
        BUILD_COMMAND make prefix=${INSTALL_PREFIX} install
        INSTALL_COMMAND ""
    )

    add_library(aio STATIC IMPORTED GLOBAL)
    SET_PROPERTY(TARGET aio PROPERTY IMPORTED_LOCATION ${INSTALL_PREFIX}/lib/libaio.a)
    file(MAKE_DIRECTORY ${INSTALL_PREFIX}/include/)
    target_include_directories(aio INTERFACE ${INSTALL_PREFIX}/include/)
    add_dependencies(aio extern_aio)
endif(LIBAIO)
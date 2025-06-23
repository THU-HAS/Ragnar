PROJECT(D-RDMA)
CMAKE_MINIMUM_REQUIRED(VERSION 2.6) 

# set(CMAKE_CXX_COMPILER "g++-7")
# set(CMAKE_CXX_COMPILER "g++-5")
set(CMAKE_CXX_COMPILER "g++")

if(NOT TARGET OpenMP::OpenMP_CXX)
    find_package(Threads REQUIRED)
    add_library(OpenMP::OpenMP_CXX IMPORTED INTERFACE)
    set_property(TARGET OpenMP::OpenMP_CXX
                 PROPERTY INTERFACE_COMPILE_OPTIONS ${OpenMP_CXX_FLAGS})
    # Only works if the same flag is passed to the linker; use CMake 3.9+ otherwise (Intel, AppleClang)
    set_property(TARGET OpenMP::OpenMP_CXX
                 PROPERTY INTERFACE_LINK_LIBRARIES ${OpenMP_CXX_FLAGS} Threads::Threads)

endif() 

# Add G_TEST SUPPORT
if($(USE_GLOG) STREQUAL "ON")
    set(GLOG_LIBRARIES 
        /usr/include/glog/vlog_is_on.h
        /usr/include/glog/log_severity.h  
        /usr/include/glog/logging.h  
        /usr/include/glog/raw_logging.h  
        /usr/include/glog/stl_logging.h)
    add_library (GLOG ${GLOG_LIBRARIES})
    add_library (GLOG ${GLOG_LIBRARIES})
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY bin/)
set(LIBRARY_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/lib/)
set(LIBMRDMA_SRC 
    ${CMAKE_SOURCE_DIR}/src/libmrdma/*
    ${CMAKE_SOURCE_DIR}/src/tool/*)
set(D-RDMA_SRC 
    ${LIBMRDMA_SRC}
    ${CMAKE_SOURCE_DIR}/src/*)

# Add CFLAGS
add_definitions(-std=c++11)
# add_definitions(-Ofast)
add_definitions(-D_GLIBCXX_USE_CXX11_ABI=0)
add_definitions(-g)
# add_definitions(-fopenmp)

aux_source_directory(src/libmrdma DIR_SRCS)

include_directories(
    ${CMAKE_SOURCE_DIR}
    ${HELLO_SOURCE_DIR}/.  
    ${D-RDMA_SRC})

set(mylib 
    ibverbs 
    rdmacm 
    # jemalloc 
    tbb
    pthread
    memcached)

set(TEST_LIST
    test_shuffle
    test_join
    )

foreach (TEST_NAME IN LISTS TEST_LIST)

    add_executable( ${TEST_NAME} test/${TEST_NAME}/test.cpp )

    target_link_libraries( 
        ${TEST_NAME}
        ${mylib}
        OpenMP::OpenMP_CXX
        )

endforeach()

execute_process(
    COMMAND make
    WORKING_DIRECTORY ./micro-bench/
    )

#add_custom_command(OUTPUT ./bin
#    COMMAND make
#    WORKING_DIRECTORY ./micro-bench/
#    )

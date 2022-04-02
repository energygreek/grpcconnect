project(grpcconnect)
cmake_minimum_required(VERSION 3.18)

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")
set(CMAKE_EXPORT_COMPILE_COMMANDS "ON")

find_package(Protobuf REQUIRED)
find_package(GRPC REQUIRED)

include_directories(${Protobuf_INCLUDE_DIRS})
include_directories(${CMAKE_CURRENT_BINARY_DIR})


find_program(GRPC_CPP_PLUGIN  grpc_cpp_plugin)

set(PROTOS ${CMAKE_CURRENT_SOURCE_DIR}/protos/grpcconnect.proto)
protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS DESCRIPTORS "grpc connect protobuf" ${PROTOS})

grpc_generate_cpp(GRPC_SRCS GRPC_HDRS ${CMAKE_CURRENT_BINARY_DIR} ${PROTOS})


# set(CMAKE_INSTALL_PREFIX /home/jimery/.local)
# set(CMAKE_INSTALL_RPATH "$ORIGIN/../lib:$ORIGIN/../lib64")
set(CMAKE_INSTALL_RPATH ${CMAKE_INSTALL_PREFIX}/lib)
# set_target_properties(myexe PROPERTIES INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
link_directories(${CMAKE_LIBRARY_OUTPUT_DIRECTORY})


add_library(grpcconnect-proto SHARED
    ${PROTO_SRCS}
    ${GRPC_SRCS}
)

target_link_libraries(grpcconnect-proto
    gRPC::grpc++
    ${Protobuf_LIBRARIES}
)

target_include_directories(grpcconnect-proto PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
set_target_properties(grpcconnect-proto
    PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

add_subdirectory(src)

if(RUN_TESTS)
    include(UnitTest)
    enable_testing()
    add_subdirectory(test)
endif()
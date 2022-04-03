cmake_minimum_required(VERSION 3.18)

add_executable(AsyncPlatform AsyncPlatform.cpp)
add_executable(Platform Platform.cpp)
add_executable(Consumer Consumer.cpp)
add_executable(Producer Producer.cpp)

target_link_libraries(AsyncPlatform grpcconnect-proto)
target_link_libraries(Platform grpcconnect-proto)
target_link_libraries(Consumer grpcconnect-proto)
target_link_libraries(Producer grpcconnect-proto)

set_target_properties(Platform Consumer Producer AsyncPlatform
    PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

install(TARGETS
    Platform Consumer Producer AsyncPlatform
    COMPONENT grpcconnect-proto
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
    )
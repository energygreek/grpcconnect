cmake_minimum_required(VERSION 3.18)

add_executable(Platform Platform.cpp)
add_executable(Consumer Consumer.cpp)
add_executable(Producer Producer.cpp)

add_executable(SyncPlatform SyncPlatform.cpp)
add_executable(SyncConsumer SyncConsumer.cpp)
add_executable(SyncProducer SyncProducer.cpp)

target_link_libraries(Platform grpcconnect-proto)
target_link_libraries(Consumer grpcconnect-proto)
target_link_libraries(Producer grpcconnect-proto)

target_link_libraries(SyncPlatform grpcconnect-proto)
target_link_libraries(SyncConsumer grpcconnect-proto)
target_link_libraries(SyncProducer grpcconnect-proto)


set_target_properties(Platform Consumer Producer SyncPlatform SyncConsumer SyncProducer
    PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

install(TARGETS
    Platform Consumer Producer SyncPlatform SyncConsumer SyncProducer
    COMPONENT grpcconnect-proto
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
    )
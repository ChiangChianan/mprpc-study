add_executable(
  user_service user_service.cpp ${CMAKE_SOURCE_DIR}/example/user.pb.cc
               ${CMAKE_SOURCE_DIR}/src/rpc_provider.cpp)

target_include_directories(user_service PRIVATE ${CMAKE_SOURCE_DIR}/example
                                                ${CMAKE_SOURCE_DIR}/include)

target_link_libraries(user_service PRIVATE mprpc protobuf)

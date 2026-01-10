add_executable(
  user_client user_client.cpp ${CMAKE_SOURCE_DIR}/example/user.pb.cc
              ${CMAKE_SOURCE_DIR}/src/rpc_consumer.cpp)

target_include_directories(user_client PUBLIC ${CMAKE_SOURCE_DIR}/include)

target_link_libraries(user_client PUBLIC mprpc protobuf)


add_executable(
  friend_client friend_client.cpp ${CMAKE_SOURCE_DIR}/example/friend.pb.cc
              ${CMAKE_SOURCE_DIR}/src/rpc_consumer.cpp)

target_include_directories(friend_client PUBLIC ${CMAKE_SOURCE_DIR}/include)

target_link_libraries(friend_client PUBLIC mprpc protobuf)

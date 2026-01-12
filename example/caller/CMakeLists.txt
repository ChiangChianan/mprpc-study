add_executable(user_client user_client.cpp ${CMAKE_SOURCE_DIR}/proto/user.pb.cc
                           ${CMAKE_SOURCE_DIR}/src/mprpc_consumer.cpp)

target_include_directories(user_client PUBLIC ${CMAKE_SOURCE_DIR}/include
                                              ${CMAKE_SOURCE_DIR}/proto)

target_link_libraries(user_client PUBLIC mprpc protobuf)

add_executable(
  friend_client friend_client.cpp ${CMAKE_SOURCE_DIR}/proto/friend.pb.cc
                ${CMAKE_SOURCE_DIR}/src/mprpc_consumer.cpp)

target_include_directories(friend_client PUBLIC ${CMAKE_SOURCE_DIR}/include
                                                ${CMAKE_SOURCE_DIR}/proto)

target_link_libraries(friend_client PUBLIC mprpc protobuf)

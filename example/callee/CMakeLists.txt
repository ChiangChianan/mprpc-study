add_executable(
  user_service user_service.cpp ${CMAKE_SOURCE_DIR}/proto/user.pb.cc
               ${CMAKE_SOURCE_DIR}/src/mprpc_provider.cpp)

target_include_directories(user_service PUBLIC ${CMAKE_SOURCE_DIR}/include ${CMAKE_SOURCE_DIR}/proto) 

target_link_libraries(user_service PUBLIC mprpc protobuf)

add_executable(
  friend_service friend_service.cpp ${CMAKE_SOURCE_DIR}/proto/friend.pb.cc
                 ${CMAKE_SOURCE_DIR}/src/mprpc_provider.cpp)

target_include_directories(friend_service PUBLIC ${CMAKE_SOURCE_DIR}/include ${CMAKE_SOURCE_DIR}/proto)

target_link_libraries(friend_service PUBLIC mprpc protobuf)

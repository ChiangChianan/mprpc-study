add_library(
  mprpc SHARED
  mprpc_application.cpp
  mprpc_config.cpp
  mprpc_provider.cpp
  mprpc_consumer.cpp
  mprpc_controller.cpp
  mprpc_logger.cpp
  zookeeper_client.cpp
  ${CMAKE_SOURCE_DIR}/proto/mprpc_header.pb.cc)

target_include_directories(
  mprpc PUBLIC ${CMAKE_SOURCE_DIR}/include ${CMAKE_SOURCE_DIR}/proto
               ${MUDUO_INCLUDE_DIR})
if(EXISTS "/usr/local/lib/libzookeeper_mt.so")
  message("libzookeeper_mt.so exists")
endif()
target_link_directories(mprpc PUBLIC ${MUDUO_LIBRARY_DIR})
target_link_libraries(mprpc muduo_net muduo_base zookeeper_mt pthread)

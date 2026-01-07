add_library(mprpc SHARED mprpc_application.cpp mprpc_config.cpp rpc_provider.cpp rpc_header.pb.cc)

target_include_directories(mprpc PUBLIC ${CMAKE_SOURCE_DIR}/include    
                                        ${CMAKE_SOURCE_DIR}/example  
                                        ${MUDUO_INCLUDE_DIR})

target_link_directories(mprpc PUBLIC ${MUDUO_LIBRARY_DIR})
target_link_libraries(mprpc muduo_net muduo_base pthread)
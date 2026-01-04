add_library(mprpc SHARED mprpc_application.cpp mprpc_config.cpp)

target_include_directories(mprpc PRIVATE ${CMAKE_SOURCE_DIR}/include)

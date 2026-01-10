#include <iostream>
#include "mprpc_application.h"
#include "rpc_consumer.h"
#include "user.pb.h"

int main(int argc, char** argv) {
  MprpcApplication::Init(argc, argv);

  example::UserServiceRpc_Stub stub(new MprpcChannel());

  example::LoginRequest request;
  request.set_name("yan hanqing");
  request.set_password("123");
  example::LoginResponse response;
  // 发起rpc方法的调用
  stub.Login(nullptr, &request, &response, nullptr);

  if (0 == response.result().error_code()) {
    std::cout << "rpc login response:" << response.success() << std::endl;
  } else {
    std::cout << "rpc login response error:"
              << response.result().error_message() << std::endl;
  }

  return 0;
}
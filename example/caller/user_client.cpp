#include <iostream>
#include "mprpc_application.h"
#include "mprpc_consumer.h"
#include "mprpc_logger.h"
#include "user.pb.h"

int main(int argc, char** argv) {
  MprpcLogger::GetInstance().Init();
  LOG_INFO("user_client start");
  MprpcApplication::Init(argc, argv);

  example::UserServiceRpc_Stub stub(new MprpcChannel());

  example::LoginRequest login_request;
  login_request.set_name("yhq");
  login_request.set_password("123");
  example::LoginResponse login_response;
  // 发起rpc方法的调用
  stub.Login(nullptr, &login_request, &login_response, nullptr);
  if (0 == login_response.result().error_code()) {
    LOG_INFO_FMT("rpc login response: %s",
                 login_response.success() ? "true" : "false");
  } else {
    LOG_ERROR_FMT("rpc login response error: %s",
                  login_response.result().error_message().c_str());
  }

  example::RegisterRequest register_request;
  register_request.set_id(123456);
  register_request.set_name("yyb");
  register_request.set_password("123456");
  example::RegisterResponse register_response;

  stub.Register(nullptr, &register_request, &register_response, nullptr);
  if (0 == register_response.result().error_code()) {
    LOG_INFO_FMT("rpc register response: %s",
                 register_response.success() ? "true" : "false");
  } else {
    LOG_ERROR_FMT("rpc register response error: %s",
                  register_response.result().error_message().c_str());
  }

  return 0;
}
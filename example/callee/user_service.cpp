#include <iostream>
#include "mprpc_application.h"
#include "mprpc_logger.h"
#include "rpc_provider.h"
#include "user.pb.h"

class UserService : public ::example::UserServiceRpc {  // rpc服务发布者
 public:
  bool Login(std::string name, std::string pwd) {
    LOG_INFO_FMT("Login attempt, username=: %s, pwd_length= %zu", name.c_str(),
                 pwd.length());
    return true;
  }

  void Login(::google::protobuf::RpcController* controller,
             const ::example::LoginRequest* request,
             ::example::LoginResponse* response,
             ::google::protobuf::Closure* done) {
    std::string name = request->name();
    std::string pwd = request->password();
    bool login_result = Login(name, pwd);
    response->set_success(login_result);
    if (login_result) {
      response->mutable_result()->set_error_code(0);
      response->mutable_result()->set_error_message("");
    } else {
      response->mutable_result()->set_error_code(1);
      response->mutable_result()->set_error_message("login failed");
    }
    done->Run();
  }

  bool Register(uint32_t id, std::string name, std::string pwd) {
    LOG_INFO_FMT("Register attempt, id=: %d, username= %s, pwd_length= %zu", id,
                 name.c_str(), pwd.length());
    return true;
  }

  void Register(::google::protobuf::RpcController* controller,
                const ::example::RegisterRequest* request,
                ::example::RegisterResponse* response,
                ::google::protobuf::Closure* done) {
    uint32_t id = request->id();
    std::string name = request->name();
    std::string pwd = request->password();
    bool register_result = Register(id, name, pwd);
    response->set_success(register_result);
    if (register_result) {
      response->mutable_result()->set_error_code(0);
      response->mutable_result()->set_error_message("");
    } else {
      response->mutable_result()->set_error_code(1);
      response->mutable_result()->set_error_message("register failed");
    }
    done->Run();
  }
};

int main(int argc, char** argv) {
  MprpcLogger::GetInstance().Init();
  LOG_INFO("user_serice start");
  // 调用框架的初始化操作
  MprpcApplication::Init(argc, argv);
  // RpcProvider是一个Rpc网络服务对象，把UserService发布到Rpc节点上
  RpcProvider service_provider;
  service_provider.NotifyService(new UserService());
  // 启动一个Rpc发布节点
  service_provider.Run();
  return 0;
}
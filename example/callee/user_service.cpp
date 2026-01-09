#include <iostream>
#include "mprpc_application.h"
#include "rpc_provider.h"
#include "user.pb.h"

class UserService : public example::UserServiceRpc {  // rpc服务发布者
 public:
  bool Login(std::string name, std::string pwd) {
    std::cout << "Do local service:" << std::endl;
    std::cout << "name:" << name << ' ' << "pwd:" << pwd << std::endl;
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

 private:
};

int main(int argc, char** argv) {
  // 调用框架的初始化操作
  MprpcApplication::Init(argc, argv);
  // RpcProvider是一个Rpc网络服务对象，把UserService发布到Rpc节点上
  RpcProvider service_provider;
  service_provider.NotifyService(new UserService());
  // 启动一个Rpc发布节点
  service_provider.Run();
  return 0;
}
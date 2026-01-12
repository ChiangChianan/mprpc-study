#include <iostream>
#include <vector>
#include "friend.pb.h"
#include "mprpc_application.h"
#include "mprpc_logger.h"
#include "mprpc_provider.h"

class FriendService : public example::FriendServiceRpc {
 public:
  std::vector<std::string> GetFriendList(uint32_t user_id) {
    LOG_INFO_FMT("GetFriendList started for user_id: %d", user_id);
    std::vector<std::string> friend_list;
    friend_list.push_back("yhq");
    friend_list.push_back("yyb");
    friend_list.push_back("wsk");
    return friend_list;
  }
  void GetFriendList(::google::protobuf::RpcController* controller,
                     const ::example::GetFriendListRequest* request,
                     ::example::GetFriendListResponse* response,
                     ::google::protobuf::Closure* done) {
    uint32_t user_id = request->user_id();
    std::vector<std::string> friend_list = GetFriendList(user_id);
    if (true) {
      for (auto& f : friend_list) {
        response->add_friends(f);
      }
      response->mutable_result()->set_error_code(0);
      response->mutable_result()->set_error_message("");
    } else {
      response->mutable_result()->set_error_code(1);
      response->mutable_result()->set_error_message("GetFriendList Failed");
    }
    done->Run();
  }
};

int main(int argc, char** argv) {
  MprpcLogger::GetInstance().Init();
  LOG_INFO("friend_service start");
  // 调用框架的初始化操作
  MprpcApplication::Init(argc, argv);
  // RpcProvider是一个Rpc网络服务对象，把UserService发布到Rpc节点上
  RpcProvider service_provider;
  service_provider.NotifyService(new FriendService());
  // 启动一个Rpc发布节点
  service_provider.Run();
  return 0;
}
#include <iostream>
#include <vector>
#include "friend.pb.h"
#include "mprpc_application.h"
#include "mprpc_controller.h"
#include "mprpc_logger.h"
#include "mprpc_consumer.h"

int main(int argc, char** argv) {
  MprpcLogger::GetInstance().Init();
  LOG_INFO("friend_client start");
  MprpcApplication::Init(argc, argv);

  example::FriendServiceRpc_Stub stub(new MprpcChannel());

  example::GetFriendListRequest get_friend_list_request;
  get_friend_list_request.set_user_id(1);
  example::GetFriendListResponse get_friend_list_response;
  MprpcController mprpc_controller;
  // 发起rpc方法的调用
  stub.GetFriendList(&mprpc_controller, &get_friend_list_request,
                     &get_friend_list_response, nullptr);
  if (mprpc_controller.Failed()) {
    LOG_ERROR(mprpc_controller.ErrorText());
  } else {
    if (0 == get_friend_list_response.result().error_code()) {
      LOG_INFO("rpc GetFriendList success");
      for (const std::string& friend_name :
           get_friend_list_response.friends()) {
        LOG_INFO_FMT("Friend: %s", friend_name.c_str());
      }
    } else {
      LOG_ERROR_FMT("rpc GetFriendList response error: %s",
                    get_friend_list_response.result().error_message().c_str());
    }
  }

  return 0;
}
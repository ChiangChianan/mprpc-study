#ifndef RPC_PROVIDER_H
#define RPC_PROVIDER_H

#include "user.pb.h"

// 框架提供的专门发布rpc服务的网络对象类
class RpcProvider {
 public:
  void NotifyService(google::protobuf::Service*);
  void Run();

 private:
};

#endif
#ifndef RPC_PROVIDER_H
#define RPC_PROVIDER_H

#include <iostream>
#include <unordered_map>
#include "mprpc_application.h"
#include "muduo/net/EventLoop.h"
#include "muduo/net/InetAddress.h"
#include "muduo/net/TcpServer.h"
#include "rpc_header.pb.h"
#include "user.pb.h"

// 框架提供的专门发布rpc服务的网络对象类
class RpcProvider {
 public:
  void NotifyService(google::protobuf::Service* server);
  void Run();

 private:
  // 组合EventLoop
  muduo::net::EventLoop event_loop_;
  // 服务信息类
  struct ServiceInfo {
    google::protobuf::Service* service;
    std::unordered_map<std::string, const google::protobuf::MethodDescriptor*>
        method_map;
  };
  // 存储注册成功的服务对象以及服务方法
  std::unordered_map<std::string, ServiceInfo> service_map_;

  // 连接回调
  void OnConnection(const muduo::net::TcpConnectionPtr& conn);
  // 消息回调
  void OnMessage(const muduo::net::TcpConnectionPtr& conn,
                 muduo::net::Buffer* buffer, muduo::Timestamp time);
  // Closure的回调操作
  void SendRpcResponse(const muduo::net::TcpConnectionPtr& conn,
                       std::shared_ptr<google::protobuf::Message> response);
};

#endif
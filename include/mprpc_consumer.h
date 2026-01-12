#ifndef RPC_CONSUMER_H
#define RPC_CONSUMER_H

#include <iostream>
#include "mprpc_header.pb.h"

// 添加 Protobuf Service 头文件
#include <google/protobuf/descriptor.h>
#include <google/protobuf/message.h>
#include <google/protobuf/service.h>


class MprpcChannel : public google::protobuf::RpcChannel {
 public:
  void CallMethod(const google::protobuf::MethodDescriptor* method,
                  google::protobuf::RpcController* controller,
                  const google::protobuf::Message* request,
                  google::protobuf::Message* response,
                  google::protobuf::Closure* done);

 private:
  std::string SendAndReceiveMessage(const char* hostname, const char* port,
                                    const std::string& send_data);
  int ConnectTcpSocket(const char* hostname, const char* port);
};

#endif
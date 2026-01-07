#include "rpc_provider.h"

void RpcProvider::NotifyService(google::protobuf::Service* service) {
  ServiceInfo service_info;
  // 获取服务对象的描述信息
  const google::protobuf::ServiceDescriptor* service_descriptor =
      service->GetDescriptor();
  // 获取服务对象的名字
  std::string service_name = service_descriptor->name();
  // 获取服务对象的方法数量
  int method_count = service_descriptor->method_count();
  std::cout << "service name:" << service_name << std::endl;
  for (int i = 0; i < method_count; i++) {
    const google::protobuf::MethodDescriptor* method_descriptor =
        service_descriptor->method(i);
    std::string method_name = method_descriptor->name();
    service_info.method_map.insert({method_name, method_descriptor});
    std::cout << "method name:" << method_name << std::endl;
  }
  service_info.service = service;
  service_map_.insert({service_name, service_info});
}

void RpcProvider::Run() {
  std::string ip =
      MprpcApplication::GetInstance().GetMprpcConfig().Get("rpc_service_ip");
  uint16_t port = atoi(MprpcApplication::GetInstance()
                           .GetMprpcConfig()
                           .Get("rpc_service_port")
                           .c_str());
  muduo::net::InetAddress address(ip, port);
  muduo::net::TcpServer server(&event_loop_, address, "provider");
  server.setConnectionCallback(
      std::bind(&RpcProvider::OnConnection, this, std::placeholders::_1));
  server.setMessageCallback(
      std::bind(&RpcProvider::OnMessage, this, std::placeholders::_1,
                std::placeholders::_2, std::placeholders::_3));
  server.setThreadNum(4);
  std::cout << "RpcProvider start service at ip:" << ip << " port:" << port
            << std::endl;
  server.start();
  event_loop_.loop();
}

// 新的socket连接回调
void RpcProvider::OnConnection(const muduo::net::TcpConnectionPtr& conn) {
  if (!conn->connected()) {
    // 与rpc连接已断开
    conn->shutdown();
  }
}

// 已建立用户的读写事件回调，如果远程有一个rpc请求，则会OnMessage方法则会回应。
void RpcProvider::OnMessage(const muduo::net::TcpConnectionPtr& conn,
                            muduo::net::Buffer* buffer, muduo::Timestamp time) {
  std::string rev_buf = buffer->retrieveAllAsString();
  // 读取字节流前4个字节的内容 (注，可能要进行字节序的转换)
  uint32_t head_size = 0;
  rev_buf.copy((char*)&head_size, 4, 0);
  // 根据head_size读取数据头的原始字节流，反序列化数据，得到rpc请求的详细内容
  std::string rpc_header_str = rev_buf.substr(4, head_size);
  mprpc::RpcHeader rpc_header;
  std::string service_name;
  std::string method_name;
  uint32_t args_size;
  if (rpc_header.ParseFromString(rpc_header_str)) {
    service_name = rpc_header.service_name();
    method_name = rpc_header.method_name();
    args_size = rpc_header.args_size();
  } else {
    std::cout << "rpc_header_str:" << rpc_header_str << "parse error!"
              << std::endl;
  }
  // 获取剩下的参数信息
  std::string args_str = rev_buf.substr(4 + head_size, args_size);
  // 打印调试信息
  std::cout << "head_size: " << head_size << std::endl;
  std::cout << "rpc_header_str: " << rpc_header_str << std::endl;
  std::cout << "service_name: " << service_name << std::endl;
  std::cout << "method_name: " << method_name << std::endl;
  std::cout << "args_size: " << args_size << std::endl;

  auto service_iter = service_map_.find(service_name);
  if (service_iter == service_map_.end()) {
    std::cout << service_name << " is not exist" << std::endl;
    return;
  }

  auto method_iter = service_iter->second.method_map.find(method_name);
  if (method_iter == service_iter->second.method_map.end()) {
    std::cout << service_name << ":" << method_name << " is not exist"
              << std::endl;
    return;
  }

  google::protobuf::Service* service = service_iter->second.service;
  const google::protobuf::MethodDescriptor* method = method_iter->second;

  // 生成RPC方法调用的请求request和响应response参数
  std::unique_ptr<google::protobuf::Message> request(
      service->GetRequestPrototype(method).New());
  if (!request->ParseFromString(args_str)) {
    std::cout << "request parse error! content: " << args_str << std::endl;
    return;  // 添加return，避免继续执行
  }

  // 使用shared_ptr创建response
  std::shared_ptr<google::protobuf::Message> response(
      service->GetResponsePrototype(method).New());

  // 创建回调，使用shared_ptr
  google::protobuf::Closure* done =
      google::protobuf::NewCallback<RpcProvider,
                                    const muduo::net::TcpConnectionPtr&,
                                    std::shared_ptr<google::protobuf::Message>>(
          this, &RpcProvider::SendRpcResponse, conn, response);

  service->CallMethod(method, nullptr, request.get(), response.get(), done);
}

void RpcProvider::SendRpcResponse(
    const muduo::net::TcpConnectionPtr& conn,
    std::shared_ptr<google::protobuf::Message> response) {
  std::string response_str;
  if (response->SerializeToString(&response_str)) {
    conn->send(response_str);
  } else {
    std::cout << "serialize response_str error" << std::endl;
  }

  conn->shutdown();
}

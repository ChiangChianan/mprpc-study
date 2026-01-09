#include "rpc_provider.h"
#include <arpa/inet.h>

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

/**
 * @brief RPC服务端的消息处理回调函数
 *
 * 当接收到客户端发送的RPC请求消息时被调用。负责解析RPC请求协议、
 * 查找对应的服务方法、执行RPC调用并设置响应回调。
 *
 * 协议格式说明:
 * +------------+-------------+----------------+
 * | 4字节头长度 | 头部数据    | 参数数据       |
 * +------------+-------------+----------------+
 * | head_size  | rpc_header  | args_str       |
 * +------------+-------------+----------------+
 *
 * 头部数据为Protobuf序列化的RpcHeader消息，包含:
 * - service_name: 服务名
 * - method_name: 方法名
 * - args_size: 参数数据长度
 *
 * @param conn 当前TCP连接指针，用于后续发送响应
 * @param buffer 接收缓冲区，包含完整的RPC请求数据
 * @param time 接收到消息的时间戳（用于调试和监控）
 */

void RpcProvider::OnMessage(const muduo::net::TcpConnectionPtr& conn,
                            muduo::net::Buffer* buffer, muduo::Timestamp time) {
  std::string rev_buf = buffer->retrieveAllAsString();
  // 读取字节流前4个字节的内容 (注，可能要进行字节序的转换)
  uint32_t head_size = 0;
  rev_buf.copy((char*)&head_size, 4, 0);

  // head_size = ntohl(head_size);  // 添加这行，网络字节序转主机字节序
  // 3. 检查数据完整性（防止粘包/拆包问题）
  if (rev_buf.size() < 4 + head_size) {
    std::cout << "Incomplete packet received, expected header size: "
              << head_size << ", but got: " << rev_buf.size() - 4 << std::endl;
    // TODO: 可以考虑关闭连接或等待更多数据
    return;
  }

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

  // 检查是否有完整的参数数据
  if (rev_buf.size() < 4 + head_size + args_size) {
    std::cout << "Incomplete args data received, expected args size: "
              << args_size << ", but got: " << rev_buf.size() - 4 - head_size
              << std::endl;
    return;
  }

  // 获取剩下的参数信息
  std::string args_str = rev_buf.substr(4 + head_size, args_size);
  // 打印调试信息
  std::cout << "====================================" << std::endl;
  std::cout << "head_size: " << head_size << std::endl;
  std::cout << "rpc_header_str: " << rpc_header_str << std::endl;
  std::cout << "service_name: " << service_name << std::endl;
  std::cout << "method_name: " << method_name << std::endl;
  std::cout << "args_size: " << args_size << std::endl;
  std::cout << "====================================" << std::endl;

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

#include "rpc_consumer.h"
#include <mprpc_application.h>
#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>

// 发送消息
void MprpcChannel::CallMethod(const google::protobuf::MethodDescriptor* method,
                              google::protobuf::RpcController* controller,
                              const google::protobuf::Message* request,
                              google::protobuf::Message* response,
                              google::protobuf::Closure* done) {
  const google::protobuf::ServiceDescriptor* service_descriptor =
      method->service();

  std::string service_name = service_descriptor->name();
  std::string method_name = method->name();
  // 获取参数的序列化字符串长度 arg_size
  std::string args_str;
  uint32_t args_size = 0;
  if (request->SerializeToString(&args_str)) {
    args_size = args_str.size();
  } else {
    std::cout << "serialize request error!" << std::endl;
    return;
  }

  mprpc::RpcHeader rpc_header;
  rpc_header.set_service_name(service_name);
  rpc_header.set_method_name(method_name);
  rpc_header.set_args_size(args_size);

  std::string rpc_header_str;
  uint32_t rpc_header_size;
  if (rpc_header.SerializeToString(&rpc_header_str)) {
    rpc_header_size = rpc_header_str.size();
  } else {
    std::cout << "serialize rpc_header error!" << std::endl;
    return;
  }

  std::string send_buffer;
  send_buffer.append(reinterpret_cast<char*>(&rpc_header_size),
                     sizeof(uint32_t));
  send_buffer.append(rpc_header_str);
  send_buffer.append(args_str);

  // 打印调试信息
  std::cout << "====================================" << std::endl;
  std::cout << "rpc_header_size: " << rpc_header_size << std::endl;
  std::cout << "rpc_header_str: " << rpc_header_str << std::endl;
  std::cout << "service_name: " << service_name << std::endl;
  std::cout << "method_name: " << method_name << std::endl;
  std::cout << "args_size: " << args_size << std::endl;
  std::cout << "====================================" << std::endl;

  // 使用socket编程来发送
  std::string ip =
      MprpcApplication::GetInstance().GetMprpcConfig().Get("rpc_service_ip");
  std::string port =
      MprpcApplication::GetInstance().GetMprpcConfig().Get("rpc_service_port");
  std::string response_str =
      SendAndReceiveMessage(ip.c_str(), port.c_str(), send_buffer);
  if (!response->ParseFromString(response_str)) {
    std::cout << "parse response error! response str:" << response_str
              << std::endl;
  }
}

std::string MprpcChannel::SendAndReceiveMessage(const char* hostname,
                                                const char* port,
                                                const std::string& send_data) {
  // 1. 创建并连接套接字
  int sockfd = ConnectTcpSocket(hostname, port);
  if (sockfd == -1) {
    return "";
  }

  // 2. 发送消息
  ssize_t bytes_sent = send(sockfd, send_data.data(), send_data.size(), 0);
  if (bytes_sent != static_cast<ssize_t>(send_data.size())) {
    perror("send failed or partial send");
    close(sockfd);
    return "";
  }

  // 3. （可选）接收响应等后续操作...
  char recv_buf[1024] = {0};
  uint32_t recv_size;
  // 接收时不设置长度是错误且危险的，会导致数据丢失或程序崩溃。
  if (-1 == (recv_size = recv(sockfd, recv_buf, sizeof(recv_buf), 0))) {
    perror("recv failed");
    close(sockfd);
    return "";
  }
  std::string response_str(recv_buf, recv_size);
  // 4. 关闭连接
  close(sockfd);
  return response_str;
}

int MprpcChannel::ConnectTcpSocket(const char* hostname, const char* port) {
  // 步骤1: 设置addrinfo
  struct addrinfo hints;
  struct addrinfo *result, *rp;
  int sockfd = -1;

  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_protocol = 0;

  // 步骤2: 执行地址解析
  // getaddrinfo() 会将这些所有可能的地址都查出来，组织成一个链表，放在result中
  int ret = getaddrinfo(hostname, port, &hints, &result);
  if (ret != 0) {
    fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(ret));
    return -1;
  }

  // 步骤3: 遍历结果链表，尝试连接
  for (rp = result; rp != NULL; rp = rp->ai_next) {
    // 3.1 创建套接字
    sockfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
    if (sockfd == -1) {
      perror("socket creation failed");
      continue;  // 这个地址不行，尝试下一个
    }

    // 3.2 尝试建立TCP连接
    if (connect(sockfd, rp->ai_addr, rp->ai_addrlen) == 0) {
      // 连接成功！跳出循环
      printf("Connected successfully to %s:%s\n", hostname, port);
      break;
    }

    // 3.3 连接失败，关闭当前套接字，继续尝试下一个地址
    perror("connect failed");
    close(sockfd);
    sockfd = -1;  // 标记为尚未成功
  }

  // 步骤4: 检查是否连接成功
  if (sockfd == -1) {
    // 遍历了所有地址都失败了
    fprintf(stderr, "Could not connect to any address for %s:%s\n", hostname,
            port);
  }

  // 步骤5: 释放getaddrinfo返回的链表内存（必须！）
  freeaddrinfo(result);

  // 步骤6: 返回套接字描述符（成功）或 -1（失败）
  return sockfd;
}

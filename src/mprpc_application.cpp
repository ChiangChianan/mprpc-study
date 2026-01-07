#include "mprpc_application.h"
#include <unistd.h>
#include <iostream>

MprpcConfig MprpcApplication::mprpc_config_;

void MprpcApplication::ShowArgsHelp() {
  std::cout << "format: command -i <configfile>" << std::endl;
}

void MprpcApplication::Init(int argc, char** argv) {
  if (argc < 2) {
    ShowArgsHelp();
    exit(EXIT_FAILURE);
  }
  int opt = 0;
  std::string config_file;
  while ((opt = getopt(argc, argv, "i:")) != -1) {
    switch (opt) {
      case 'i':
        config_file = optarg;
        break;
      case '?':
        ShowArgsHelp();
        break;
      case ':':
        ShowArgsHelp();
        break;
      default:
        break;
    }
  }
  mprpc_config_.LoadConfigFile(config_file.c_str());
  std::cout << "rpc_service_ip = " << mprpc_config_.Get("rpc_service_ip")
            << std::endl;
  std::cout << "rpc_service_port = " << mprpc_config_.Get("rpc_service_port")
            << std::endl;
  std::cout << "zookeeper_ip = " << mprpc_config_.Get("zookeeper_ip")
            << std::endl;
  std::cout << "zookeeper_port = " << mprpc_config_.Get("zookeeper_port")
            << std::endl;
}

MprpcApplication& MprpcApplication::GetInstance() {
  static MprpcApplication app;
  return app;
}

MprpcConfig& MprpcApplication::GetMprpcConfig() { return mprpc_config_; }

MprpcApplication::MprpcApplication() {}
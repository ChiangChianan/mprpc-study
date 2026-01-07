#ifndef MPRPC_APPLICATION_H
#define MPRPC_APPLICATION_H

#include "mprpc_config.h"

// 用于框架初始化操作
class MprpcApplication {
 public:
  static void Init(int argc, char** argv);
  static MprpcApplication& GetInstance();
  static MprpcConfig& GetMprpcConfig();

 private:
  static MprpcConfig mprpc_config_;

  MprpcApplication();
  MprpcApplication(const MprpcApplication&) = delete;
  MprpcApplication(MprpcApplication&&) = delete;
  MprpcApplication& operator=(const MprpcApplication&) = delete;
  MprpcApplication& operator=(MprpcApplication&&) = delete;
  static void ShowArgsHelp();
};

#endif
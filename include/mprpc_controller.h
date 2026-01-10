#ifndef MPRPC_CONTROLLER_H
#define MPRPC_CONTROLLER_H

#include <google/protobuf/service.h>
#include <iostream>

class MprpcController : public ::google::protobuf::RpcController {
 public:
  MprpcController();
  virtual ~MprpcController() = default;

  void Reset() override;
  bool Failed() const override;
  std::string ErrorText() const override;
  void StartCancel() override;
  void SetFailed(const std::string& reason) override;
  bool IsCanceled() const override;
  void NotifyOnCancel(::google::protobuf::Closure* callback) override;

 private:
  bool failed_ = false;        // 是否失败
  bool canceled_ = false;      // 是否已取消
  std::string error_message_;  // 改为_message更清晰
};

#endif
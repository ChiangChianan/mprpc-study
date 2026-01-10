#include "mprpc_controller.h"

MprpcController::MprpcController() : failed_(false), canceled_(false) {
  // 可以生成默认请求ID
  // request_id_ = GenerateUUID();
}

void MprpcController::Reset() {
  failed_ = false;
  canceled_ = false;
  error_message_.clear();
}

bool MprpcController::Failed() const { return failed_; }

std::string MprpcController::ErrorText() const {
  return error_message_;  // 这里返回成员变量
}

void MprpcController::SetFailed(const std::string& reason) {
  failed_ = true;
  error_message_ = reason;
  // 可以记录日志
  std::cerr << "[RPC Failed] " << reason << std::endl;
}

void MprpcController::StartCancel() { canceled_ = true; }

bool MprpcController::IsCanceled() const { return canceled_; }

void MprpcController::NotifyOnCancel(::google::protobuf::Closure* callback) {}
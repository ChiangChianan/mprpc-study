#include "mprpc_config.h"
#include <cstring>
#include <iostream>

std::string MprpcConfig::Trim(const std::string& str) {
  size_t start = str.find_first_not_of(" \t\r\n");
  if (start == std::string::npos) {
    return "";  // 全是空白字符
  }
  size_t end = str.find_last_not_of(" \t\r\n");
  return str.substr(start, end - start + 1);
}

bool MprpcConfig::ParseConfigLine(const std::string& line) {
  // 查找等号位置
  size_t pos = line.find('=');
  if (pos == std::string::npos) {
    return false;  // 没有等号，格式错误
  }
  // 分离键和值
  std::string key = Trim(line.substr(0, pos));
  std::string value = Trim(line.substr(pos + 1));
  // 检查键是否为空
  if (key.empty()) {
    return false;
  }
  // 存储配置
  config_map_[key] = value;
  return true;
}

bool MprpcConfig::LoadConfigFile(const char* config_file) {
  FILE* pf = fopen(config_file, "r");
  if (nullptr == pf) {
    std::cerr << "Error: Cannot open config file: " << config_file << std::endl;
    return false;
  }
  char buf[512];
  while (fgets(buf, sizeof(buf), pf) != nullptr) {
    // 移除换行符
    size_t len = strlen(buf);
    if (len > 0 && buf[len - 1] == '\n') {
      buf[len - 1] = '\0';
    }

    // 去除字符串首尾空格
    std::string line = Trim(buf);

    // 跳过空行和注释行
    if (line.empty() || line[0] == '#') {
      continue;
    }

    // 解析配置项
    if (!ParseConfigLine(line)) {
      std::cerr << "Warning: Invalid config line: " << line << std::endl;
    }
  }

  fclose(pf);  // 必须关闭文件
  return true;
}

std::string MprpcConfig::Get(const std::string& key) const {
  auto it = config_map_.find(key);
  if (it == config_map_.end()) {
    return "";
  }
  return it->second;
}
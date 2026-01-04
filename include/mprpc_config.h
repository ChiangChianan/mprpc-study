#ifndef MPRPC_CONFIG_H
#define MPRPC_CONFIG_H
#include <string>
#include <unordered_map>

class MprpcConfig {
 public:
  std::string Trim(const std::string& str);
  bool ParseConfigLine(const std::string& line);
  bool LoadConfigFile(const char* config_file);
  std::string Get(const std::string& key) const;

 private:
  std::unordered_map<std::string, std::string> config_map_;
};

#endif
#include "zookeeper_client.h"
#include <stdlib.h>
#include <string.h>
#include <stdexcept>

#include "mprpc_application.h"
#include "mprpc_logger.h"

// 全局观察器
void ZooKeeperClient::global_watcher(zhandle_t *zh, int type, int state,
                                     const char *path, void *watcherCtx) {
  if (watcherCtx == nullptr) return;

  ZooKeeperClient *client = static_cast<ZooKeeperClient *>(watcherCtx);
  client->HandleWatcherEvent(type, state, path);
}

ZooKeeperClient::ZooKeeperClient() : zhandle_(nullptr), connected_(false) {
  sem_init(&connected_sem_, 0, 0);
}

ZooKeeperClient::~ZooKeeperClient() {
  if (zhandle_ != nullptr) {
    zookeeper_close(zhandle_);
    zhandle_ = nullptr;
  }
  sem_destroy(&connected_sem_);
}

void ZooKeeperClient::HandleWatcherEvent(int type, int state,
                                         const char *path) {
  if (type == ZOO_SESSION_EVENT) {
    if (state == ZOO_CONNECTED_STATE) {
      connected_ = true;
      sem_post(&connected_sem_);
      LOG_INFO_FMT("Connected to ZooKeeper server!");
    } else if (state == ZOO_EXPIRED_SESSION_STATE) {
      connected_ = false;
      LOG_ERROR_FMT("Session expired! Need to reconnect.");
      // 这里可以触发重连逻辑
    } else if (state == ZOO_CONNECTING_STATE) {
      LOG_INFO_FMT("Connecting to ZooKeeper...");
    } else if (state == ZOO_ASSOCIATING_STATE) {
      LOG_INFO_FMT("Associating with ZooKeeper...");
    }
  } else if (type == ZOO_CREATED_EVENT) {
    LOG_INFO_FMT("Node created: %s", path);
  } else if (type == ZOO_DELETED_EVENT) {
    LOG_INFO_FMT("Node deleted: %s", path);
  } else if (type == ZOO_CHANGED_EVENT) {
    LOG_INFO_FMT("Node changed: %s", path);
  } else if (type == ZOO_CHILD_EVENT) {
    LOG_INFO_FMT("Children changed: %s", path);
  }
}

bool ZooKeeperClient::Start() {
  std::string ip =
      MprpcApplication::GetInstance().GetMprpcConfig().Get("zookeeper_ip");
  std::string port =
      MprpcApplication::GetInstance().GetMprpcConfig().Get("zookeeper_port");
  std::string conn_str = ip + ":" + port;

  // 设置超时时间，可从配置读取
  int timeout = 30000;

  zhandle_ = zookeeper_init(conn_str.c_str(), global_watcher, timeout, nullptr,
                            this, 0);
  if (nullptr == zhandle_) {
    LOG_ERROR_FMT("zookeeper init error");
    return false;
  }

  // 等待连接建立，设置超时
  struct timespec ts;
  if (clock_gettime(CLOCK_REALTIME, &ts) == -1) {
    return false;
  }
  ts.tv_sec += 10;  // 10秒超时

  if (sem_timedwait(&connected_sem_, &ts) == -1) {
    LOG_ERROR_FMT("Connect to ZooKeeper timeout");
    zookeeper_close(zhandle_);
    zhandle_ = nullptr;
    return false;
  }

  LOG_INFO_FMT("zookeeper init success");
  return true;
}

bool ZooKeeperClient::Create(const char *path, const char *data, int data_len,
                             int state) {
  if (!connected_) {
    LOG_ERROR_FMT("ZooKeeper not connected");
    return false;
  }

  // 检查节点是否存在
  int flag = zoo_exists(zhandle_, path, 0, nullptr);
  if (flag == ZOK) {
    LOG_INFO_FMT("Node already exists: %s", path);
    return true;  // 节点已存在，不算错误
  } else if (flag != ZNONODE) {
    LOG_ERROR_FMT("Check node exists error: %s, error: %d", path, flag);
    return false;
  }

  // 创建节点
  std::vector<char> path_buffer(strlen(path) + 256);  // 动态分配
  int buffer_len = path_buffer.size();

  flag = zoo_create(zhandle_, path, data, data_len, &ZOO_OPEN_ACL_UNSAFE, state,
                    path_buffer.data(), buffer_len);

  if (flag == ZOK) {
    LOG_INFO_FMT("znode create success... path: %s", path);
    return true;
  } else {
    LOG_ERROR_FMT("znode create error... path: %s, error: %s", path,
                  zerror(flag));
    return false;
  }
}

bool ZooKeeperClient::GetData(const char *path, std::string &data) {
  if (!connected_) {
    LOG_ERROR("ZooKeeper not connected");
    return false;
  }

  // 合理大小的初始缓冲区（大部分节点数据小于4KB）
  std::vector<char> buffer(4096);
  int buffer_len = buffer.size();

  int flag = zoo_get(zhandle_, path, 0, buffer.data(), &buffer_len, nullptr);

  if (flag == ZOK) {
    if (buffer_len <= buffer.size()) {
      // 获取完整数据
      data.assign(buffer.data(), buffer_len);
      return true;
    } else {
      // 数据比预期大，重新获取
      if (buffer_len > 10 * 1024 * 1024) {  // 限制10MB
        LOG_ERROR_FMT("Data too large (%d bytes) for node %s", buffer_len,
                      path);
        return false;
      }

      buffer.resize(buffer_len);
      buffer_len = buffer.size();
      flag = zoo_get(zhandle_, path, 0, buffer.data(), &buffer_len, nullptr);

      if (flag == ZOK) {
        data.assign(buffer.data(), buffer_len);
        return true;
      }
    }
  }

  // 错误处理
  if (flag == ZNONODE) {
    LOG_ERROR_FMT("Node %s does not exist", path);
  } else {
    LOG_ERROR_FMT("Failed to get data from %s: %s", path, zerror(flag));
  }

  return false;
}

bool ZooKeeperClient::SetData(const char *path, const char *data,
                              int data_len) {
  if (!connected_) {
    LOG_ERROR_FMT("ZooKeeper not connected");
    return false;
  }

  int flag = zoo_set(zhandle_, path, data, data_len, -1);
  if (flag == ZOK) {
    LOG_INFO_FMT("znode set success... path: %s", path);
    return true;
  } else {
    LOG_ERROR_FMT("znode set error... path: %s, error: %s", path, zerror(flag));
    return false;
  }
}

bool ZooKeeperClient::Exists(const char *path) {
  if (!connected_) {
    LOG_ERROR_FMT("ZooKeeper not connected");
    return false;
  }

  int flag = zoo_exists(zhandle_, path, 0, nullptr);
  return (flag == ZOK);
}

#ifndef ZOOKEEPER_CLIENT_H
#define ZOOKEEPER_CLIENT_H

#include <semaphore.h>
#include <zookeeper/zookeeper.h>
#include <atomic>
#include <iostream>
#include <vector>

class ZooKeeperClient {
 public:
  ZooKeeperClient();
  ~ZooKeeperClient();

  bool Start();
  bool Create(const char *path, const char *data, int data_len, int state = 0);
  bool GetData(const char *path, std::string &data);
  bool SetData(const char *path, const char *data, int data_len);
  bool Exists(const char *path);
  bool Delete(const char *path, int version = -1);
  bool GetChildren(const char *path, std::vector<std::string> &children);

  bool IsConnected() const { return connected_; }

  void HandleWatcherEvent(int type, int state, const char *path);

 private:
  static void global_watcher(zhandle_t *zh, int type, int state,
                             const char *path, void *watcherCtx);

 private:
  zhandle_t *zhandle_;
  sem_t connected_sem_;
  std::atomic<bool> connected_;
};

#endif
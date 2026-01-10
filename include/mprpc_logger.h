#ifndef MPRPC_LOGGER_H
#define MPRPC_LOGGER_H

#include <atomic>
#include <condition_variable>
#include <fstream>
#include <iostream>
#include <mutex>
#include <queue>
#include <thread>

// 日志级别
enum class LogLevel { DEBUG = 0, INFO, WARN, ERROR, FATAL };

// 日志消息结构
struct LogMessage {
  std::string timestamp;  // 时间戳
  LogLevel level;         // 日志级别
  std::string message;    // 日志内容
  std::thread::id tid;    // 线程ID

  LogMessage(const std::string& ts, const LogLevel& l, const std::string& msg,
             std::thread::id id)
      : timestamp(ts), level(l), message(msg), tid(id) {}
};

// 线程安全的阻塞队列
template <typename T>
class BlockingQueue {
 public:
  BlockingQueue(size_t max_size = 10000)
      : max_size_(max_size), shutdown_(false) {}
  ~BlockingQueue() { Shutdown(); }

  bool Push(const T& item, bool wait = true) {
    std::unique_lock<std::mutex> lock(mutex_);
    if (wait) {
      // 等待队列有空闲空间
      not_full_.wait(
          lock, [this]() { return queue_.size() < max_size_ || shutdown_; });
    }
    if (shutdown_) return false;
    if (queue_.size() >= max_size_) return false;
    queue_.push(item);
    not_empty_.notify_one();
    return true;
  }

  bool Pop(T& item, bool wait = true) {
    std::unique_lock<std::mutex> lock(mutex_);
    if (wait) {
      // 等待队列有数据
      not_empty_.wait(lock, [this]() { return !queue_.empty() || shutdown_; });
    }
    if (queue_.empty()) return false;
    item = std::move(queue_.front());
    queue_.pop();
    not_full_.notify_one();
    return true;
  }

  // 批量取出消息
  bool PopBatch(std::vector<T>& items, size_t max_count, bool wait = true) {
    std::unique_lock<std::mutex> lock(mutex_);
    if (wait) {
      not_empty_.wait(lock, [this]() { return !queue_.empty() || shutdown_; });
    }
    if (queue_.empty()) return false;
    while (!queue_.empty() && items.size() < max_count) {
      items.emplace_back(std::move(queue_.front()));
      queue_.pop();
    }
    if (!queue_.empty()) {
      not_full_.notify_one();
    }
    return true;
  }

  size_t Size() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return queue_.size();
  }

  bool Empty() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return queue_.empty();
  }

  void Shutdown() {
    std::lock_guard<std::mutex> lock(mutex_);
    shutdown_ = true;
    not_full_.notify_all();
    not_empty_.notify_all();
  }

 private:
  mutable std::mutex mutex_;
  std::condition_variable not_empty_;
  std::condition_variable not_full_;
  std::queue<T> queue_;
  size_t max_size_;
  std::atomic<bool> shutdown_;
};

// 日志模块单例类
class MprpcLogger {
 public:
  // 获取单例实例
  static MprpcLogger& GetInstance();

  // 初始化日志系统
  void Init(LogLevel level = LogLevel::INFO,
            const std::string& log_dir = "./logs", bool console_output = true,
            size_t queue_size = 10000, bool async = false);

  // 记录日志（同步）
  void LogSync(LogLevel level, const std::string& message);

  // 记录日志（异步）
  void LogAsync(LogLevel level, const std::string& message);

  // 便捷日志方法
  void Debug(const std::string& message);
  void Info(const std::string& message);
  void Warn(const std::string& message);
  void Error(const std::string& message);
  void Fatal(const std::string& message);

  // 设置日志级别
  void SetLogLevel(LogLevel level);

  // 停止日志系统
  void Shutdown();

  // 获取日志队列当前大小
  size_t GetQueueSize() const { return log_queue_.Size(); }

 private:
  MprpcLogger();
  ~MprpcLogger();

  // 禁止拷贝构造
  MprpcLogger(const MprpcLogger&) = delete;
  MprpcLogger& operator=(MprpcLogger&) = delete;

  // 禁止移动构造
  MprpcLogger(MprpcLogger&&) = delete;
  MprpcLogger& operator=(MprpcLogger&&) = delete;

  // 获取当前时间字符串
  std::string GetCurrentTime();

  // 获取当前日期字符串（用于文件名）
  std::string GetCurrentDate();

  // 获取日志级别字符串
  std::string GetLevelString(LogLevel level);

  // 打开新的日志文件
  void OpenNewLogFile();

  // 后台线程工作函数
  void WriteThreadFunc();

  // 写入单条日志
  void WriteLog(const LogMessage& log_msg);

  // 批量写入日志
  void WriteLogBatch(const std::vector<LogMessage>& log_msgs);

 private:
  LogLevel level_;            //日志级别
  std::ofstream log_file_;    // 日志文件流
  std::string log_dir_;       // 日志目录
  std::string current_date_;  // 当前日志文件日期
  bool console_output_;       // 是否输出到控制台
  bool initialized_;          // 是否已初始化
  bool async_;                // 是否异步模式

  // 线程安全队列
  BlockingQueue<LogMessage> log_queue_;
  std::mutex init_mutex_;

  // 后台写线程
  std::thread write_thread_;
  std::atomic<bool> running_{false};
  std::atomic<bool> shutdown_{false};

  // 批量写入相关
  size_t batch_size_{100};                        // 批量写入大小
  std::chrono::milliseconds batch_timeout_{100};  // 批量写入超时时间
};

#define LOG_DEBUG(message) MprpcLogger::GetInstance().Debug(message);
#define LOG_INFO(message) MprpcLogger::GetInstance().Info(message)
#define LOG_WARN(message) MprpcLogger::GetInstance().Warn(message)
#define LOG_ERROR(message) MprpcLogger::GetInstance().Error(message)
#define LOG_FATAL(message) MprpcLogger::GetInstance().Fatal(message)

// 带格式的日志宏
#define LOG_DEBUG_FMT(fmt, ...)                           \
  do {                                                    \
    char buffer[1024];                                    \
    snprintf(buffer, sizeof(buffer), fmt, ##__VA_ARGS__); \
    MprpcLogger::GetInstance().Debug(buffer);             \
  } while (0)

#define LOG_INFO_FMT(fmt, ...)                            \
  do {                                                    \
    char buffer[1024];                                    \
    snprintf(buffer, sizeof(buffer), fmt, ##__VA_ARGS__); \
    MprpcLogger::GetInstance().Info(buffer);              \
  } while (0)

#define LOG_WARN_FMT(fmt, ...)                            \
  do {                                                    \
    char buffer[1024];                                    \
    snprintf(buffer, sizeof(buffer), fmt, ##__VA_ARGS__); \
    MprpcLogger::GetInstance().Warn(buffer);              \
  } while (0)

#define LOG_ERROR_FMT(fmt, ...)                           \
  do {                                                    \
    char buffer[1024];                                    \
    snprintf(buffer, sizeof(buffer), fmt, ##__VA_ARGS__); \
    MprpcLogger::GetInstance().Error(buffer);             \
  } while (0)

#define LOG_FATAL_FMT(fmt, ...)                           \
  do {                                                    \
    char buffer[1024];                                    \
    snprintf(buffer, sizeof(buffer), fmt, ##__VA_ARGS__); \
    MprpcLogger::GetInstance().Fatal(buffer);             \
  } while (0)

#endif
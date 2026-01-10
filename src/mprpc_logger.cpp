#include "mprpc_logger.h"
#include <sys/stat.h>
#include <sys/types.h>
#include <cerrno>
#include <cstring>
#include <ctime>
#include <iomanip>
#include <sstream>

MprpcLogger::MprpcLogger()
    : level_(LogLevel::INFO),
      console_output_(true),
      initialized_(false),
      async_(false),
      log_queue_(10000) {}

MprpcLogger::~MprpcLogger() { Shutdown(); }

MprpcLogger& MprpcLogger::GetInstance() {
  static MprpcLogger instance;
  return instance;
}

void MprpcLogger::Init(LogLevel level, const std::string& log_dir,
                       bool console_output, size_t queue_size, bool async) {
  std::lock_guard<std::mutex> lock(init_mutex_);

  if (initialized_) {
    // 如果已经初始化，记录警告信息 简单、安全地输出到控制台
    std::cerr << "[WARN] MprpcLogger already initialized." << std::endl;
    return;
  }
  level_ = level;
  log_dir_ = log_dir;
  console_output_ = console_output;
  async_ = async;
  if (mkdir(log_dir.c_str(), 0755) != 0 && errno != EEXIST) {
    std::cerr << "[Error] Failed to create log directory: " << log_dir_
              << ", error: " << strerror(errno) << std::endl;
    return;
  }

  // 打开今天的日志文件
  OpenNewLogFile();
  initialized_ = true;
  // 启动后台写线程
  if (async_) {
    running_ = true;
    shutdown_ = false;
    write_thread_ = std::thread(&MprpcLogger::WriteThreadFunc, this);

    // 记录初始化日志
    LogAsync(LogLevel::INFO,
             "Async logger initialized. Log level: " + GetLevelString(level_) +
                 ", Log directory: " + log_dir_ +
                 ", Queue size: " + std::to_string(queue_size));
  } else {
    LogSync(LogLevel::INFO,
            "Sync logger initialized. Log level: " + GetLevelString(level) +
                ", Log directory: " + log_dir_);
  }
}

void MprpcLogger::LogSync(LogLevel level, const std::string& message) {
  // 检查日志级别
  if (level < level_) {
    return;
  }

  // 如果没有初始化，先初始化
  if (!initialized_) {
    Init();  // 使用默认参数初始化
  }

  // 获取时间戳和线程ID
  std::string timestamp = GetCurrentTime();
  std::thread::id tid = std::this_thread::get_id();

  // 创建日志消息
  LogMessage log_msg(timestamp, level, message, tid);

  // 直接写入
  WriteLog(log_msg);
}

void MprpcLogger::LogAsync(LogLevel level, const std::string& message) {
  // 检查日志级别
  if (level < level_) {
    return;
  }
  // 如果没有初始化，先初始化
  if (!initialized_) {
    Init();  // 使用默认参数初始化
  }
  // 获取时间戳和线程ID
  std::string timestamp = GetCurrentTime();
  std::thread::id tid = std::this_thread::get_id();

  // 创建日志消息
  LogMessage log_msg(timestamp, level, message, tid);

  // 放入队列（非阻塞）
  if (!log_queue_.Push(log_msg, false)) {
    // 队列满了，降级为同步写入或丢弃
    if (level >= LogLevel::ERROR) {
      // 重要错误日志，同步写入
      WriteLog(log_msg);
    }
    // 其他日志直接丢弃，避免阻塞调用线程
  }
}

void MprpcLogger::OpenNewLogFile() {
  // 关闭当前文件
  if (log_file_.is_open()) {
    log_file_.flush();
    log_file_.close();
  }
  current_date_ = GetCurrentDate();
  std::string filename = log_dir_ + "/mprpc_" + current_date_ + ".log";
  log_file_.open(filename, std::ios_base::out | std::ios_base::app);
  if (!log_file_.is_open()) {
    std::cerr << "Failed to open log file: " << filename << std::endl;
    return;
  }
  // 写入文件头（如果文件是新创建的）
  if (log_file_.tellp() == 0) {
    log_file_ << "========== MP-RPC Log File ==========\n";
    log_file_ << "Created: " << GetCurrentTime() << "\n";
    log_file_ << "=====================================\n\n";
    log_file_.flush();
  }
}

std::string MprpcLogger::GetCurrentDate() {
  auto now = std::chrono::system_clock::now();
  auto time = std::chrono::system_clock::to_time_t(now);
  std::tm tm_info;
  localtime_r(&time, &tm_info);
  std::stringstream ss;
  ss << std::put_time(&tm_info, "%Y-%m-%d");
  return ss.str();
}

std::string MprpcLogger::GetCurrentTime() {
  auto now = std::chrono::system_clock::now();
  auto time = std::chrono::system_clock::to_time_t(now);
  // 获得当前的毫秒数
  auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                now.time_since_epoch()) %
            1000;
  std::tm tm_info;
  localtime_r(&time, &tm_info);
  std::stringstream ss;
  ss << std::put_time(&tm_info, "%Y-%m-%d %H:%M:%S");
  ss << '.' << std::setfill('0') << std::setw(3) << ms.count();
  return ss.str();
}

std::string MprpcLogger::GetLevelString(LogLevel level) {
  switch (level) {
    case LogLevel::DEBUG:
      return "DEBUG";
    case LogLevel::INFO:
      return "INFO";
    case LogLevel::WARN:
      return "WARN";
    case LogLevel::ERROR:
      return "ERROR";
    case LogLevel::FATAL:
      return "FATAL";
    default:
      return "UNKNOWN";
  }
}

void MprpcLogger::WriteThreadFunc() {
  std::vector<LogMessage> batch;
  batch.reserve(batch_size_);

  while (running_ || !log_queue_.Empty()) {
    batch.clear();

    // 批量从队列取出日志
    if (log_queue_.PopBatch(batch, batch_size_, running_)) {
      if (!batch.empty()) {
        WriteLogBatch(batch);
      }
    }

    // 检查是否需要切换日志文件（跨天了）
    std::string today = GetCurrentDate();
    if (today != current_date_) {
      OpenNewLogFile();
    }

    // 短暂休眠，避免CPU空转
    if (log_queue_.Empty()) {
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
  }
}

void MprpcLogger::WriteLog(const LogMessage& log_msg) {
  // 检查是否需要创建新的日志文件（跨天了）
  std::string today = GetCurrentDate();
  if (today != current_date_) {
    OpenNewLogFile();
  }
  // 构建日志字符串
  std::stringstream ss;
  ss << log_msg.timestamp << " [" << GetLevelString(log_msg.level)
     << "] [TID:" << log_msg.tid << "] " << log_msg.message << "\n";

  std::string log_str = ss.str();
  // 输出到控制台
  if (console_output_) {
    // 为不同级别设置颜色（Unix/Linux终端）
#ifndef _WIN32
    switch (log_msg.level) {
      case LogLevel::DEBUG:
        std::cout << "\033[36m";
        break;  // 青色
      case LogLevel::INFO:
        std::cout << "\033[32m";
        break;  // 绿色
      case LogLevel::WARN:
        std::cout << "\033[33m";
        break;  // 黄色
      case LogLevel::ERROR:
        std::cout << "\033[31m";
        break;  // 红色
      case LogLevel::FATAL:
        std::cout << "\033[35m";
        break;  // 紫色
      default:
        break;
    }
#endif

    std::cout << log_str;

    std::cout << "\033[0m";  // 重置颜色
    std::cout.flush();
  }

  // 写入文件
  if (log_file_.is_open()) {
    log_file_ << log_str;
    log_file_.flush();
  }
}

void MprpcLogger::WriteLogBatch(const std::vector<LogMessage>& log_msgs) {
  if (log_msgs.empty()) return;
  // 检查是否需要创建新的日志文件（跨天了）
  std::string today = GetCurrentDate();
  if (today != current_date_) {
    OpenNewLogFile();
  }
  std::stringstream ss;

  // 构建批量日志字符串
  for (const auto& log_msg : log_msgs) {
    ss << log_msg.timestamp << " [" << GetLevelString(log_msg.level)
       << "] [TID:" << log_msg.tid << "] " << log_msg.message << "\n";
  }

  std::string batch_log_str = ss.str();

  // 输出到控制台
  if (console_output_) {
    std::cout << batch_log_str;
    std::cout.flush();
  }

  // 批量写入文件
  if (log_file_.is_open()) {
    log_file_ << batch_log_str;
    log_file_.flush();
  }
}

void MprpcLogger::Debug(const std::string& message) {
  if (async_) {
    LogAsync(LogLevel::DEBUG, message);
  } else {
    LogSync(LogLevel::DEBUG, message);
  }
}

void MprpcLogger::Info(const std::string& message) {
  if (async_) {
    LogAsync(LogLevel::INFO, message);
  } else {
    LogSync(LogLevel::INFO, message);
  }
}

void MprpcLogger::Warn(const std::string& message) {
  if (async_) {
    LogAsync(LogLevel::WARN, message);
  } else {
    LogSync(LogLevel::WARN, message);
  }
}

void MprpcLogger::Error(const std::string& message) {
  if (async_) {
    LogAsync(LogLevel::ERROR, message);
  } else {
    LogSync(LogLevel::ERROR, message);
  }
}

void MprpcLogger::Fatal(const std::string& message) {
  if (async_) {
    LogAsync(LogLevel::FATAL, message);
  } else {
    LogSync(LogLevel::FATAL, message);
  }
}

void MprpcLogger::SetLogLevel(LogLevel level) {
  level_ = level;
  Info("Log level changed to: " + GetLevelString(level));
}

void MprpcLogger::Shutdown() {
  if (!initialized_) return;

  // 标记关闭
  shutdown_ = true;
  running_ = false;

  // 停止队列
  log_queue_.Shutdown();

  // 等待写线程结束
  if (write_thread_.joinable()) {
    write_thread_.join();
  }

  // 关闭文件
  if (log_file_.is_open()) {
    log_file_.flush();
    log_file_.close();
  }

  initialized_ = false;

  // 记录关闭日志（直接输出到控制台）
  std::cout << GetCurrentTime() << " [INFO] Logger shutdown completed\n";
}
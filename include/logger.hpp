#pragma once

#include <cerrno>
#include <chrono>
#include <cstring>
#include <filesystem>
#include <format>
#include <fstream>
#include <iostream>
#include <mutex>
#include <source_location>
#include <sstream>
#include <string_view>

enum class LogLevel
{
  Debug,
  Info,
  Warning,
  Error
};

constexpr std::string_view log_level_to_string(const LogLevel level)
{
  switch (level)
  {
  case LogLevel::Debug:
    return "DEBUG";
  case LogLevel::Info:
    return "INFO ";
  case LogLevel::Warning:
    return "WARN ";
  case LogLevel::Error:
    return "ERROR";
  }
  return "?????";
}

class Logger
{
public:
  Logger(const Logger &) = delete;
  Logger &operator=(const Logger &) = delete;
  Logger(Logger &&) = delete;
  Logger &operator=(Logger &&) = delete;

  static Logger &get_instance()
  {
    static Logger instance;
    return instance;
  }

  template <typename... Args>
  void log(const LogLevel level, const std::source_location loc = std::source_location::current(), const Args &...args)
  {
    if (!log_file_stream)
    {
      return;
    }

    std::lock_guard<std::mutex> lock(mutex);

    const auto now = std::chrono::floor<std::chrono::milliseconds>(std::chrono::system_clock::now());

    std::ostringstream oss;
    oss << std::format("{:%Y-%m-%d %H:%M:%S}", now) << " [" << log_level_to_string(level) << "] " << loc.file_name()
        << ":" << loc.line() << ": ";
    (oss << ... << args);

    log_file_stream << oss.str() << std::endl;
    log_file_stream.flush();
  }

private:
  std::ofstream log_file_stream;
  std::mutex mutex;

  Logger()
  {
    const char *xdg_state_home = std::getenv("XDG_STATE_HOME");
    const std::filesystem::path state_home =
        (xdg_state_home ? xdg_state_home : std::filesystem::path(std::getenv("HOME")) / ".local/state");
    const std::filesystem::path log_dir = state_home / "tower";
    const std::filesystem::path log_file_path = log_dir / "tower.log";

    std::error_code ec;
    std::filesystem::create_directories(log_dir, ec);
    if (ec)
    {
      std::cerr << "failed to create tower log directory " << log_dir << ": " << ec.message() << "\n";
      return;
    }

    log_file_stream.open(log_file_path, std::ios::out | std::ios::app);
    if (!log_file_stream.is_open())
    {
      std::cerr << "failed to write to " << log_file_path << ": " << std::strerror(errno) << "\n";
      return;
    }

    log_file_stream << '\n';
  }

  ~Logger()
  {
    if (log_file_stream.is_open())
    {
      log_file_stream.close();
    }
  }
};

inline Logger &logger() { return Logger::get_instance(); }

#define LOG_INFO(...) logger().log(LogLevel::Info, std::source_location::current(), __VA_ARGS__)
#define LOG_WARNING(...) logger().log(LogLevel::Warning, std::source_location::current(), __VA_ARGS__)
#define LOG_ERROR(...) logger().log(LogLevel::Error, std::source_location::current(), __VA_ARGS__)

#ifndef NDEBUG
#define LOG_DEBUG(...) logger().log(LogLevel::Debug, std::source_location::current(), __VA_ARGS__)
#else
#define LOG_DEBUG(...) ((void)0)
#endif

#include "Logger.h"

#include <fmt/chrono.h>

#include <chrono>
#include <cstdio>
#include <mutex>

#include "Core/Error.h"

namespace Rastery {
static std::mutex gMutex; // mutex for thread-safe logging.
static Logger *gLogger;   // singleton logger.

constexpr std::string_view gTimeFormat = "{:%T}";
constexpr std::string_view gLogFormat = "[{level:<15} {time:>16}] {message}\n";
Logger::Logger() = default;

static std::string getLevelName(Logger::Level level) {
  switch (level) {
  case Logger::Level::Debug:
    return "\033[34mDebug\033[0m";
  case Logger::Level::Info:
    return "\033[32mInfo\033[0m";
  case Logger::Level::Warning:
    return "\033[33mWarn\033[0m";
  case Logger::Level::Error:
    return "\033[31mError\033[0m";
  case Logger::Level::Fatal:
    return "\033[31mFatal\033[0m";
  default:
    return "\033[37mUnknown\033[0m";
  }
}

static std::string
formatLogMessage(std::chrono::time_point<std::chrono::system_clock> time,
                 Logger::Level level, std::string_view message) {
  return fmt::format(gLogFormat, fmt::arg("level", getLevelName(level)),
                     fmt::arg("time", fmt::format(gTimeFormat, time)),
                     fmt::arg("message", message));
}

static void writeToFile(FILE *fd, std::string_view message) {
  RASTERY_ASSERT(fd != nullptr);
  fmt::print(fd, "{}", message.data());
  // flush immediately to avoid losing log messages.
  std::fflush(fd);
}
/**
 * @brief print log message to stdout before logger initialization.
 */
void logBeforeInitialized(Logger::Level level, std::string_view message) {
  auto time = std::chrono::system_clock::now();
  writeToFile(stdout, formatLogMessage(time, level, message));
  if (level == Logger::Level::Fatal)
    std::exit(1);
}

void Logger::init() { gLogger = new Logger(); }

void Logger::log(Level level, std::string_view message) {
  if (gLogger == nullptr) {
    logBeforeInitialized(Logger::Level::Fatal, "Logger not initialized.");
    return;
  }
  std::string formattedMessage(
      formatLogMessage(std::chrono::system_clock::now(), level, message));
  std::lock_guard<std::mutex> lock(gMutex);

  writeToFile(stdout, formattedMessage);
}

void Logger::shutdown() { delete gLogger; }

using Level = Logger::Level;
} // namespace Rastery
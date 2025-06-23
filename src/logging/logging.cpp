#include "logging.h"
#include <fmt/core.h>
#include <iostream>

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
static LogLevel g_minLogLevel = LogLevel::Info;

void setLogLevel(LogLevel logLevel) { g_minLogLevel = logLevel; }

LogLevel parseLogLevel(const std::string &levelStr) {
  static const std::unordered_map<std::string, LogLevel> map = {
      {"debug", LogLevel::Debug}, {"info", LogLevel::Info},
      {"warn", LogLevel::Warn},   {"warning", LogLevel::Warn},
      {"error", LogLevel::Error}, {"none", LogLevel::None}};

  auto levelIterator = map.find(levelStr);
  if (levelIterator != map.end()) {
    return levelIterator->second;
  }
  std::cerr << "Invalid log level, defaulting to info\n";
  return LogLevel::Info;
}

bool shouldLog(QtMsgType type) {
  switch (g_minLogLevel) {
  case LogLevel::Debug:
    return true;
  case LogLevel::Info:
    return type != QtDebugMsg;
  case LogLevel::Warn:
    return type == QtWarningMsg || type == QtCriticalMsg || type == QtFatalMsg;
  case LogLevel::Error:
    return type == QtCriticalMsg || type == QtFatalMsg;
  case LogLevel::None:
    return type == QtFatalMsg;
  default:
    return true;
  }
}

void messageHandler(QtMsgType type, const QMessageLogContext &context,
                    const QString &msg) {
  if (!shouldLog(type)) {
    return;
  }

  static QMutex mutex;
  QMutexLocker lock(&mutex);

  std::string prefix;

  switch (type) {
  case QtDebugMsg:
    prefix = "DEBUG";
    break;
  case QtInfoMsg:
    prefix = "INFO";
    break;
  case QtWarningMsg:
    prefix = "WARNING";
    break;
  case QtCriticalMsg:
    prefix = "CRITICAL";
    break;
  case QtFatalMsg:
    prefix = "FATAL";
    break;
  }
  std::string timestamp = QDateTime::currentDateTime()
                              .toString("yyyy-MM-dd hh:mm:ss.zzz")
                              .toStdString();

  std::string logMessage;

#ifdef QT_MESSAGELOGCONTEXT
  try {
    logMessage = fmt::format("[{}][{}] {} ({}:{}:{})", timestamp, prefix,
                             msg.toStdString(), context.file, context.line,
                             context.function);
  } catch (...) {
    logMessage = msg.toStdString();
  }
#else
  try {
    logMessage =
        fmt::format("[{}][{}] {}", timestamp, prefix, msg.toStdString());
  } catch (...) {
    logMessage = msg.toStdString();
  }
#endif

  if (type == QtDebugMsg || type == QtInfoMsg) {
    std::cout << logMessage.c_str() << '\n';
  } else {
    std::cerr << logMessage.c_str() << '\n';
  }

  if (type == QtFatalMsg) {
    abort();
  }
}
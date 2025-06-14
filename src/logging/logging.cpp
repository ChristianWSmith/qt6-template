#include "logging.h"
#include <fmt/core.h>

static LogLevel g_minLogLevel = LogLevel::Info;

void setLogLevel(LogLevel logLevel) { g_minLogLevel = logLevel; }

LogLevel parseLogLevel(const std::string &levelStr) {
  static const std::unordered_map<std::string, LogLevel> map = {
      {"debug", LogLevel::Debug}, {"info", LogLevel::Info},
      {"warn", LogLevel::Warn},   {"warning", LogLevel::Warn},
      {"error", LogLevel::Error}, {"none", LogLevel::None}};

  auto it = map.find(levelStr);
  if (it != map.end()) {
    return it->second;
  }
  throw std::invalid_argument("Invalid log level: " + levelStr);
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
  if (!shouldLog(type))
    return;

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

#ifdef QT_MESSAGELOGCONTEXT
  std::string logMessage = fmt::format("[{}][{}] {} ({}:{}:{})", timestamp,
                                       prefix, msg.toStdString(), context.file,
                                       context.line, context.function);
#else
  std::string logMessage =
      fmt::format("[{}][{}] {}", timestamp, prefix, msg.toStdString());
#endif

  if (type == QtDebugMsg || type == QtInfoMsg) {
    fprintf(stdout, "%s\n", logMessage.c_str());
  } else {
    fprintf(stderr, "%s\n", logMessage.c_str());
  }

  if (type == QtFatalMsg) {
    abort();
  }
}
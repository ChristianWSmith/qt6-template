#include "messageHandler.h"
#include <fmt/core.h>

void messageHandler(QtMsgType type, const QMessageLogContext &context,
                    const QString &msg) {

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

  std::string logMessage = fmt::format("[{}][{}] {} ({}:{}:{})", timestamp,
                                       prefix, msg.toStdString(), context.file,
                                       context.line, context.function);

  if (type == QtDebugMsg || type == QtInfoMsg) {
    fprintf(stdout, "%s\n", logMessage.c_str());
  } else {
    fprintf(stderr, "%s\n", logMessage.c_str());
  }

  if (type == QtFatalMsg) {
    abort();
  }
}
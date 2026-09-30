#include "logging.h"
#include <fmt/format.h>
#include <iostream>

Q_LOGGING_CATEGORY(appMain, "app.main")
Q_LOGGING_CATEGORY(appFeature, "app.feature")
Q_LOGGING_CATEGORY(appPersistence, "app.persistence")
Q_LOGGING_CATEGORY(appEvent, "app.event")
Q_LOGGING_CATEGORY(appService, "app.service")

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

  std::string logMessage;

#ifdef QT_MESSAGELOGCONTEXT
  logMessage = fmt::format("[{}][{}] {} ({}:{}:{})", timestamp, prefix,
                           msg.toStdString(), context.file, context.line,
                           context.function);
#else
  logMessage =
      fmt::format("[{}][{}] {}", timestamp, prefix, msg.toStdString());
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

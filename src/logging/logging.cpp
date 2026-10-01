#include "logging.h"
#include <fmt/format.h>
#include <iostream>

Q_LOGGING_CATEGORY(appMain, "app.main")
Q_LOGGING_CATEGORY(appFeature, "app.feature")
Q_LOGGING_CATEGORY(appPersistence, "app.persistence")
Q_LOGGING_CATEGORY(appEvent, "app.event")
Q_LOGGING_CATEGORY(appService, "app.service")

// F-11: messageHandler is RETAINED deliberately.
//
// Why not replace with qSetMessagePattern + default Qt handler:
// - Pedagogical demonstration of qInstallMessageHandler + fmt in a second,
//   realistic call site (main.cpp keeps the Hello-message fmt demo).
// - Explicit stdout/stderr routing (debug/info -> stdout; warning+ -> stderr).
//   Qt's default handler writes all levels to stderr; pattern formatting does
//   not restore this split.
// - Explicit fatal contract: QtFatalMsg -> abort() is visible in template code,
//   not implicit behind Qt internals.
// - Deterministic console sinks via std::cout/std::cerr when a console is
//   attached (debug/diagnostic runs; secondary rationale only).
//
// What qSetMessagePattern CAN produce if the template later prefers the
// canonical formatting path (categories + configureLogLevel stay either way):
//   qSetMessagePattern(
//       "[%{time datetime yyyy-MM-dd hh:mm:ss.zzz}][%{type}] %{message}"
//       " (%{file}:%{line}:%{function})");
// Differences vs this handler: lowercase %{type} names, all output on stderr,
// no fatal abort visible in application code, no category in the line
// (current handler also omits category; filtering is via configureLogLevel).
//
// Category note: this handler does not print QMessageLogContext::category.
// Category *filtering* is the canonical, kept mechanism (configureLogLevel /
// QLoggingCategory::setFilterRules). Add %{category} via pattern, or extend
// this handler, only if line-level category display becomes a requirement.
//
// fmt note: fmt is a documented Conan demonstration (AGENTS.md Keep list).
// Losing this site would leave only main.cpp's Hello message as the demo.
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

void configureLogLevel(const std::string &levelStr) {
  if (levelStr.empty()) {
    return;
  }

  QStringList rules;
  if (levelStr == "debug") {
    rules << "*.debug=true" << "*.info=true" << "*.warning=true"
          << "*.critical=true";
  } else if (levelStr == "info") {
    rules << "*.debug=false" << "*.info=true" << "*.warning=true"
          << "*.critical=true";
  } else if (levelStr == "warn" || levelStr == "warning") {
    rules << "*.debug=false" << "*.info=false" << "*.warning=true"
          << "*.critical=true";
  } else if (levelStr == "error") {
    rules << "*.debug=false" << "*.info=false" << "*.warning=false"
          << "*.critical=true";
  } else if (levelStr == "none") {
    rules << "*.debug=false" << "*.info=false" << "*.warning=false"
          << "*.critical=false";
  } else {
    qCWarning(appMain) << "Unknown log level; leaving Qt logging rules"
                       << "unchanged:" << levelStr.c_str();
    return;
  }
  QLoggingCategory::setFilterRules(rules.join('\n'));
}

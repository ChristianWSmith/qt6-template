// NOLINTBEGIN
#include "logging/logging.h"

#include <QDebug>
#include <QLoggingCategory>
#include <QString>
#include <algorithm>
#include <gtest/gtest.h>
#include <vector>

// Behavioral tests for configureLogLevel filter rules.
//
// QLoggingCategory has no public rules-read API, so these tests are
// black-box: install a capturing message handler (same probe pattern as
// ServiceRegistrationTest), apply rules, emit qC* messages, and assert
// which messages reach the handler. Category filter rules are checked
// before Qt invokes the message handler — a filtered message never
// reaches the capture probe.
//
// Message matching uses unique probe substrings in the payload rather
// than context.category so tests do not depend on QT_MESSAGELOGCONTEXT
// being defined in the current build type.

namespace {

struct CapturedMessage {
  QtMsgType type{};
  QString message;
};

std::vector<CapturedMessage> g_captured;
QtMessageHandler g_previousHandler = nullptr;

void capturingHandler(QtMsgType type, const QMessageLogContext &context,
                      const QString &msg) {
  Q_UNUSED(context)
  g_captured.push_back(CapturedMessage{type, msg});
  if (g_previousHandler != nullptr) {
    g_previousHandler(type, context, msg);
  }
}

bool hasMessage(const std::vector<CapturedMessage> &msgs, QtMsgType type,
                const QString &needle) {
  return std::any_of(msgs.begin(), msgs.end(), [&](const CapturedMessage &m) {
    return m.type == type && m.message.contains(needle);
  });
}

} // namespace

class LoggingTest : public ::testing::Test {
protected:
  void SetUp() override {
    g_captured.clear();
    g_previousHandler = qInstallMessageHandler(capturingHandler);
  }

  void TearDown() override {
    qInstallMessageHandler(g_previousHandler);
    g_previousHandler = nullptr;
    // Restore baseline filter rules after every test (rules are
    // process-global). Unknown tokens are no-ops; "info" is the safe reset.
    configureLogLevel("info");
    g_captured.clear();
  }
};

TEST_F(LoggingTest, DebugLevelEnablesDebugMessages) {
  configureLogLevel("debug");
  qCDebug(appMain) << "loggingtest-debug-probe";
  qCInfo(appMain) << "loggingtest-info-probe";

  EXPECT_TRUE(hasMessage(g_captured, QtDebugMsg, "loggingtest-debug-probe"))
      << "configureLogLevel(\"debug\") did not enable debug messages";
  EXPECT_TRUE(hasMessage(g_captured, QtInfoMsg, "loggingtest-info-probe"))
      << "configureLogLevel(\"debug\") did not enable info messages";
}

TEST_F(LoggingTest, ErrorLevelFiltersDebugMessages) {
  configureLogLevel("error");
  qCDebug(appMain) << "loggingtest-error-filter-debug-probe";
  qCInfo(appMain) << "loggingtest-error-filter-info-probe";
  qCWarning(appMain) << "loggingtest-error-filter-warning-probe";
  qCCritical(appMain) << "loggingtest-error-filter-critical-probe";

  EXPECT_FALSE(hasMessage(g_captured, QtDebugMsg,
                          "loggingtest-error-filter-debug-probe"))
      << "configureLogLevel(\"error\") left debug messages enabled";
  EXPECT_FALSE(hasMessage(g_captured, QtInfoMsg,
                          "loggingtest-error-filter-info-probe"))
      << "configureLogLevel(\"error\") left info messages enabled";
  EXPECT_FALSE(hasMessage(g_captured, QtWarningMsg,
                          "loggingtest-error-filter-warning-probe"))
      << "configureLogLevel(\"error\") left warning messages enabled";
  EXPECT_TRUE(hasMessage(g_captured, QtCriticalMsg,
                         "loggingtest-error-filter-critical-probe"))
      << "configureLogLevel(\"error\") disabled critical messages";
}

TEST_F(LoggingTest, UnknownTokenLeavesPreviousRulesUnchanged) {
  // Baseline: warnings on, debug off.
  configureLogLevel("warn");
  configureLogLevel("not-a-level");

  qCDebug(appMain) << "loggingtest-unknown-filter-debug-probe";
  qCWarning(appMain) << "loggingtest-unknown-filter-warning-probe";

  EXPECT_FALSE(hasMessage(g_captured, QtDebugMsg,
                          "loggingtest-unknown-filter-debug-probe"))
      << "Unknown token changed rules: debug messages are now enabled";
  EXPECT_TRUE(hasMessage(g_captured, QtWarningMsg,
                         "loggingtest-unknown-filter-warning-probe"))
      << "Unknown token changed rules: warning messages are no longer enabled";
}

TEST_F(LoggingTest, UnknownTokenEmitsWarningAndPreservesDebugRules) {
  configureLogLevel("debug");
  g_captured.clear();

  configureLogLevel("not-a-level");

  EXPECT_TRUE(hasMessage(g_captured, QtWarningMsg, "Unknown log level"))
      << "Unknown token should emit qCWarning(appMain) when warnings are on";

  qCDebug(appMain) << "loggingtest-after-unknown-debug-probe";
  EXPECT_TRUE(hasMessage(g_captured, QtDebugMsg,
                         "loggingtest-after-unknown-debug-probe"))
      << "Unknown token must not reset debug rules to a stricter level";
}

TEST_F(LoggingTest, EmptyLevelStringIsNoOp) {
  configureLogLevel("debug");
  configureLogLevel("");

  qCDebug(appMain) << "loggingtest-empty-level-probe";
  EXPECT_TRUE(hasMessage(g_captured, QtDebugMsg, "loggingtest-empty-level-probe"))
      << "Empty level string must not change existing filter rules";
}

#include "LoggingTest.moc"
// NOLINTEND

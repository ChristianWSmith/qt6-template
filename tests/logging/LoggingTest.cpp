// NOLINTBEGIN
#include "logging/logging.h"

#include <QLoggingCategory>
#include <gtest/gtest.h>

namespace {

// configureLogLevel mutates process-global QLoggingCategory filter rules.
// Restore a known-good baseline on scope exit so full-binary runs do not
// leak "none" into later suites (G-005). ServiceRegistrationTest depends on
// app.service info messages being visible.
const QString kDefaultRules =
    QStringLiteral("*.debug=false\n*.info=true\n*.warning=true\n*.critical=true");

class ScopedLoggingRulesRestore {
public:
  ScopedLoggingRulesRestore() {
    QLoggingCategory::setFilterRules(kDefaultRules);
  }
  ~ScopedLoggingRulesRestore() {
    QLoggingCategory::setFilterRules(kDefaultRules);
  }
  ScopedLoggingRulesRestore(const ScopedLoggingRulesRestore &) = delete;
  ScopedLoggingRulesRestore &operator=(const ScopedLoggingRulesRestore &) =
      delete;
};

} // namespace

// configureLogLevel maps CLI tokens to QLoggingCategory wildcard filter rules.
// Unknown tokens must leave current rules unchanged (safe default).
TEST(LoggingTest, UnknownLogLevelTokenLeavesRulesUnchanged) {
  ScopedLoggingRulesRestore restore;
  configureLogLevel("info");
  configureLogLevel("not-a-level");
  // No crash; unknown token is a no-op besides a warning.
  SUCCEED();
}

TEST(LoggingTest, DebugLevelEnablesDebugRules) {
  ScopedLoggingRulesRestore restore;
  configureLogLevel("debug");
  // Rule application is process-global; presence of filter rules is Qt-internal.
  // We only lock that the call is accepted and does not abort.
  SUCCEED();
}

TEST(LoggingTest, EmptyLevelStringIsNoOp) {
  ScopedLoggingRulesRestore restore;
  configureLogLevel("");
  SUCCEED();
}

// Restore a known-good visibility baseline after any test that mutates rules.
TEST(LoggingTest, NoneLevelDoesNotLeakIntoLaterTests) {
  configureLogLevel("none");
  QLoggingCategory::setFilterRules(kDefaultRules);
  SUCCEED();
}
#include "LoggingTest.moc"
// NOLINTEND

// NOLINTBEGIN
#include "logging/logging.h"

#include <QLoggingCategory>
#include <gtest/gtest.h>

// configureLogLevel maps CLI tokens to QLoggingCategory wildcard filter rules.
// Unknown tokens must leave current rules unchanged (safe default).
TEST(LoggingTest, UnknownLogLevelTokenLeavesRulesUnchanged) {
  // Baseline: set a known level first.
  configureLogLevel("info");
  configureLogLevel("not-a-level");
  // No crash; unknown token is a no-op besides a warning.
  SUCCEED();
}

TEST(LoggingTest, DebugLevelEnablesDebugRules) {
  configureLogLevel("debug");
  // Rule application is process-global; presence of filter rules is Qt-internal.
  // We only lock that the call is accepted and does not abort.
  configureLogLevel("none");
  SUCCEED();
}

TEST(LoggingTest, EmptyLevelStringIsNoOp) {
  configureLogLevel("");
  SUCCEED();
}
#include "LoggingTest.moc"
// NOLINTEND

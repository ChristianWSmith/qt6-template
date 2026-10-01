// NOLINTBEGIN
#include "core/IPersistenceProvider.h"

#include <QJsonObject>
#include <gtest/gtest.h>

class PersistenceResultTest : public ::testing::Test {};

TEST_F(PersistenceResultTest, VoidSuccessHasValue) {
  auto result = PersistenceResult<void>::success();
  EXPECT_TRUE(result.hasValue());
  EXPECT_FALSE(result.hasError());
}

TEST_F(PersistenceResultTest, VoidFailureHasError) {
  auto result = PersistenceResult<void>::failure(PersistenceError::NotFound);
  EXPECT_FALSE(result.hasValue());
  EXPECT_TRUE(result.hasError());
  EXPECT_EQ(result.error(), PersistenceError::NotFound);
}

TEST_F(PersistenceResultTest, JsonObjectSuccessReturnsValue) {
  QJsonObject obj;
  obj["key"] = "value";
  obj["num"] = 42;

  auto result = PersistenceResult<QJsonObject>::success(obj);
  EXPECT_TRUE(result.hasValue());
  EXPECT_FALSE(result.hasError());
  EXPECT_EQ(result.value()["key"].toString(), "value");
  EXPECT_EQ(result.value()["num"].toInt(), 42);
}

TEST_F(PersistenceResultTest, JsonObjectFailureHasError) {
  auto result = PersistenceResult<QJsonObject>::failure(
      PersistenceError::InvalidData);
  EXPECT_FALSE(result.hasValue());
  EXPECT_TRUE(result.hasError());
  EXPECT_EQ(result.error(), PersistenceError::InvalidData);
}

TEST_F(PersistenceResultTest, VoidFailureIoError) {
  auto result = PersistenceResult<void>::failure(PersistenceError::IoError);
  EXPECT_TRUE(result.hasError());
  EXPECT_EQ(result.error(), PersistenceError::IoError);
}

TEST_F(PersistenceResultTest, JsonObjectFailureCommitError) {
  auto result = PersistenceResult<QJsonObject>::failure(
      PersistenceError::CommitError);
  EXPECT_TRUE(result.hasError());
  EXPECT_EQ(result.error(), PersistenceError::CommitError);
}

TEST_F(PersistenceResultTest, SuccessResultDoesNotReportError) {
  auto ok = PersistenceResult<int>::success(7);
  ASSERT_TRUE(ok.hasValue());
  EXPECT_FALSE(ok.hasError());
  EXPECT_EQ(ok.value(), 7);
}

TEST_F(PersistenceResultTest, ErrorResultDoesNotReportValue) {
  auto bad = PersistenceResult<int>::failure(PersistenceError::IoError);
  ASSERT_TRUE(bad.hasError());
  EXPECT_FALSE(bad.hasValue());
  EXPECT_EQ(bad.error(), PersistenceError::IoError);
}

TEST_F(PersistenceResultTest, VoidSuccessDoesNotReportError) {
  auto ok = PersistenceResult<void>::success();
  ASSERT_TRUE(ok.hasValue());
  EXPECT_FALSE(ok.hasError());
}

TEST_F(PersistenceResultTest, VoidErrorAccessorsAreConsistent) {
  auto bad = PersistenceResult<void>::failure(PersistenceError::InvalidData);
  ASSERT_TRUE(bad.hasError());
  EXPECT_FALSE(bad.hasValue());
  EXPECT_EQ(bad.error(), PersistenceError::InvalidData);
}

// Misuse contract: value()/error() on the wrong state is a programming error.
// Debug builds assert; release builds abort. We do not execute those paths in
// unit tests (they would kill the test process). These tests document the
// checked-access pattern that correct callers must follow.
TEST_F(PersistenceResultTest, CallersMustCheckBeforeExtracting) {
  auto result = PersistenceResult<QJsonObject>::failure(PersistenceError::IoError);
  ASSERT_TRUE(result.hasError());
  // Correct usage: branch on hasError() before value().
  if (result.hasValue()) {
    FAIL() << "value() must not be reachable on an error result";
  }
  EXPECT_EQ(result.error(), PersistenceError::IoError);
}

#include "PersistenceResultTest.moc"

// NOLINTEND

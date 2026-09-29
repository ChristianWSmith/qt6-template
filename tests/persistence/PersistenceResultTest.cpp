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
  EXPECT_EQ((*result)["key"].toString(), "value");
  EXPECT_EQ((*result)["num"].toInt(), 42);
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

TEST_F(PersistenceResultTest, JsonObjectFailureDurabilityFailure) {
  auto result = PersistenceResult<QJsonObject>::failure(
      PersistenceError::DurabilityFailure);
  EXPECT_TRUE(result.hasError());
  EXPECT_EQ(result.error(), PersistenceError::DurabilityFailure);
}

#include "PersistenceResultTest.moc"

// NOLINTEND

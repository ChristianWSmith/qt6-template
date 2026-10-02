// NOLINTBEGIN
#include "MemoryPersistenceProvider.h"

#include <QJsonArray>
#include <QJsonObject>
#include <gtest/gtest.h>

// Contract tests for the MemoryPersistenceProvider test double (G-007).
// Locks NotFound / round-trip / injection behavior that feature tests rely on.

class MemoryProviderContractTest : public ::testing::Test {
protected:
  MemoryPersistenceProvider provider;
};

TEST_F(MemoryProviderContractTest, MissingKeyReturnsNotFound) {
  auto result = provider.loadState("missing.key");
  ASSERT_TRUE(result.hasError());
  EXPECT_EQ(result.error(), PersistenceError::NotFound);
}

TEST_F(MemoryProviderContractTest, SaveThenLoadRoundTripsJsonObject) {
  QJsonObject state;
  state.insert("value", 42);
  const auto saved = provider.saveState("round.trip", state);
  ASSERT_TRUE(saved.hasValue());

  auto loaded = provider.loadState("round.trip");
  ASSERT_TRUE(loaded.hasValue());
  EXPECT_EQ(loaded.value().value("value").toInt(), 42);
}

TEST_F(MemoryProviderContractTest, FailNextLoadIsOneShot) {
  provider.failNextLoad(PersistenceError::IoError);
  auto first = provider.loadState("any");
  ASSERT_TRUE(first.hasError());
  EXPECT_EQ(first.error(), PersistenceError::IoError);

  // Second load without injection: NotFound (key never saved).
  auto second = provider.loadState("any");
  ASSERT_TRUE(second.hasError());
  EXPECT_EQ(second.error(), PersistenceError::NotFound);
}

TEST_F(MemoryProviderContractTest, FailNextSaveIsOneShot) {
  provider.failNextSave(PersistenceError::CommitError);
  QJsonObject state;
  auto first = provider.saveState("k", state);
  ASSERT_TRUE(first.hasError());
  EXPECT_EQ(first.error(), PersistenceError::CommitError);

  auto second = provider.saveState("k", state);
  ASSERT_TRUE(second.hasValue());
}

TEST_F(MemoryProviderContractTest, FailNextSaveCanInjectInvalidData) {
  provider.failNextSave(PersistenceError::InvalidData);
  QJsonObject state;
  auto result = provider.saveState("inject.invalid", state);
  ASSERT_TRUE(result.hasError());
  EXPECT_EQ(result.error(), PersistenceError::InvalidData);
}

TEST_F(MemoryProviderContractTest, KeysAreIsolated) {
  QJsonObject a;
  a.insert("id", 1);
  QJsonObject b;
  b.insert("id", 2);
  ASSERT_TRUE(provider.saveState("key.a", a).hasValue());
  ASSERT_TRUE(provider.saveState("key.b", b).hasValue());

  auto ra = provider.loadState("key.a");
  auto rb = provider.loadState("key.b");
  ASSERT_TRUE(ra.hasValue());
  ASSERT_TRUE(rb.hasValue());
  EXPECT_EQ(ra.value().value("id").toInt(), 1);
  EXPECT_EQ(rb.value().value("id").toInt(), 2);
}

#include "MemoryProviderContractTest.moc"

// NOLINTEND

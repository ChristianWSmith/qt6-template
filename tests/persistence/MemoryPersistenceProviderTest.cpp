// NOLINTBEGIN
#include "MemoryPersistenceProvider.h"

#include <QJsonArray>
#include <QJsonObject>
#include <gtest/gtest.h>

// Parity coverage for MemoryPersistenceProvider load taxonomy against
// FilePersistenceProvider (PersistenceTest / PersistenceLoadTest):
// non-object JSON and corrupt JSON → InvalidData; missing key → NotFound.

class MemoryPersistenceProviderTest : public ::testing::Test {
protected:
  MemoryPersistenceProvider provider;
};

TEST_F(MemoryPersistenceProviderTest, NonObjectJsonArrayReturnsInvalidData) {
  provider.seedRaw("key", QByteArrayLiteral("[1,2,3]"));
  auto result = provider.loadState("key");
  ASSERT_TRUE(result.hasError());
  EXPECT_EQ(result.error(), PersistenceError::InvalidData);
}

TEST_F(MemoryPersistenceProviderTest, NonObjectJsonScalarReturnsInvalidData) {
  provider.seedRaw("key", QByteArrayLiteral("42"));
  auto result = provider.loadState("key");
  ASSERT_TRUE(result.hasError());
  EXPECT_EQ(result.error(), PersistenceError::InvalidData);
}

TEST_F(MemoryPersistenceProviderTest, CorruptJsonReturnsInvalidData) {
  provider.seedRaw("key", QByteArrayLiteral("{ this is not valid json"));
  auto result = provider.loadState("key");
  ASSERT_TRUE(result.hasError());
  EXPECT_EQ(result.error(), PersistenceError::InvalidData);
}

TEST_F(MemoryPersistenceProviderTest, NullJsonReturnsInvalidData) {
  provider.seedRaw("key", QByteArrayLiteral("null"));
  auto result = provider.loadState("key");
  ASSERT_TRUE(result.hasError());
  EXPECT_EQ(result.error(), PersistenceError::InvalidData);
}

TEST_F(MemoryPersistenceProviderTest, MissingKeyReturnsNotFound) {
  auto result = provider.loadState("__absent_key__");
  ASSERT_TRUE(result.hasError());
  EXPECT_EQ(result.error(), PersistenceError::NotFound);
}

TEST_F(MemoryPersistenceProviderTest, ObjectRoundTripStillSucceeds) {
  QJsonObject obj;
  obj["n"] = 7;
  ASSERT_TRUE(provider.saveState("k", obj).hasValue());
  auto result = provider.loadState("k");
  ASSERT_TRUE(result.hasValue());
  EXPECT_EQ(result.value()["n"].toInt(), 7);
}

TEST_F(MemoryPersistenceProviderTest, FailNextLoadStillInjects) {
  QJsonObject obj;
  obj["x"] = 1;
  ASSERT_TRUE(provider.saveState("k", obj).hasValue());

  provider.failNextLoad(PersistenceError::IoError);
  auto injected = provider.loadState("k");
  ASSERT_TRUE(injected.hasError());
  EXPECT_EQ(injected.error(), PersistenceError::IoError);

  // One-shot injection: subsequent load succeeds.
  auto again = provider.loadState("k");
  ASSERT_TRUE(again.hasValue());
  EXPECT_EQ(again.value()["x"].toInt(), 1);
}

TEST_F(MemoryPersistenceProviderTest, FailNextSaveStillInjects) {
  QJsonObject obj;
  provider.failNextSave(PersistenceError::CommitError);
  auto injected = provider.saveState("k", obj);
  ASSERT_TRUE(injected.hasError());
  EXPECT_EQ(injected.error(), PersistenceError::CommitError);

  auto ok = provider.saveState("k", obj);
  EXPECT_TRUE(ok.hasValue());
}

#include "MemoryPersistenceProviderTest.moc"

// NOLINTEND

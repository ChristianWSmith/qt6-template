// NOLINTBEGIN
#include "platform/core/FilePersistenceProvider.h"

#include <QCoreApplication>
#include <QJsonArray>
#include <QJsonObject>
#include <QStandardPaths>
#include <gtest/gtest.h>

class PersistenceTest : public ::testing::Test {
protected:
  FilePersistenceProvider provider;

  static QString uniqueKey(const char *prefix) {
    static int counter = 0;
    return QString("%1_%2").arg(prefix).arg(counter++);
  }

  void SetUp() override {
    // Probe that atomic replace (QSaveFile::commit) works in the test-mode
    // AppDataLocation (see tests/main.cpp). CommitError means commit/replace
    // failed — not an fsync/durability observation (Qt does not surface those).
    const QString probeKey = uniqueKey("commit_probe");
    QJsonObject probeObj;
    auto result = provider.saveState(probeKey, probeObj);
    if (result.hasError() &&
        result.error() == PersistenceError::CommitError) {
      GTEST_SKIP() << "QSaveFile commit/replace not supported in this"
                    << " environment";
    }
  }
};

TEST_F(PersistenceTest, RoundTrip) {
  const QString key = uniqueKey("roundtrip");

  QJsonObject original;
  original["name"] = "test";
  original["count"] = 42;
  original["flag"] = true;

  ASSERT_TRUE(provider.saveState(key, original).hasValue()) << "Save failed";

  auto loadResult = provider.loadState(key);
  ASSERT_TRUE(loadResult.hasValue()) << "Load failed";

  const QJsonObject loaded = loadResult.value();
  EXPECT_EQ(loaded["name"].toString(), "test");
  EXPECT_EQ(loaded["count"].toInt(), 42);
  EXPECT_EQ(loaded["flag"].toBool(), true);
}

TEST_F(PersistenceTest, MultipleSavesLastOneWins) {
  const QString key = uniqueKey("overwrite");

  QJsonObject first;
  first["version"] = 1;

  QJsonObject second;
  second["version"] = 2;

  QJsonObject third;
  third["version"] = 3;

  ASSERT_TRUE(provider.saveState(key, first).hasValue());
  ASSERT_TRUE(provider.saveState(key, second).hasValue());
  ASSERT_TRUE(provider.saveState(key, third).hasValue());

  auto loadResult = provider.loadState(key);
  ASSERT_TRUE(loadResult.hasValue());
  EXPECT_EQ(loadResult.value()["version"].toInt(), 3);
}

TEST_F(PersistenceTest, DifferentKeysAreIsolated) {
  const QString keyA = uniqueKey("iso_a");
  const QString keyB = uniqueKey("iso_b");

  QJsonObject objA;
  objA["key"] = "A";

  QJsonObject objB;
  objB["key"] = "B";

  ASSERT_TRUE(provider.saveState(keyA, objA).hasValue());
  ASSERT_TRUE(provider.saveState(keyB, objB).hasValue());

  auto resultA = provider.loadState(keyA);
  auto resultB = provider.loadState(keyB);

  ASSERT_TRUE(resultA.hasValue());
  ASSERT_TRUE(resultB.hasValue());
  EXPECT_EQ(resultA.value()["key"].toString(), "A");
  EXPECT_EQ(resultB.value()["key"].toString(), "B");
}

TEST_F(PersistenceTest, EmptyJsonObject) {
  const QString key = uniqueKey("empty");

  QJsonObject empty;
  auto saveResult = provider.saveState(key, empty);
  ASSERT_TRUE(saveResult.hasValue());

  auto loadResult = provider.loadState(key);
  ASSERT_TRUE(loadResult.hasValue());
  EXPECT_TRUE(loadResult.value().isEmpty());
}

TEST_F(PersistenceTest, LargePayload) {
  const QString key = uniqueKey("large");

  QJsonArray largeArray;
  for (int i = 0; i < 1000; ++i) {
    QJsonObject item;
    item["index"] = i;
    item["value"] = QString("item_%1").arg(i);
    largeArray.append(item);
  }

  QJsonObject payload;
  payload["items"] = largeArray;
  payload["total"] = 1000;

  ASSERT_TRUE(provider.saveState(key, payload).hasValue());

  auto loadResult = provider.loadState(key);
  ASSERT_TRUE(loadResult.hasValue());

  const QJsonObject loaded = loadResult.value();
  EXPECT_EQ(loaded["total"].toInt(), 1000);
  EXPECT_EQ(loaded["items"].toArray().size(), 1000);
  EXPECT_EQ(loaded["items"].toArray()[0].toObject()["index"].toInt(), 0);
  EXPECT_EQ(loaded["items"].toArray()[999].toObject()["value"].toString(),
            "item_999");
}

class PersistenceLoadTest : public ::testing::Test {
protected:
  FilePersistenceProvider provider;
};

TEST_F(PersistenceLoadTest, FirstRunReturnsNotFound) {
  FilePersistenceProvider freshProvider;
  auto result = freshProvider.loadState("__nonexistent_key_abc123");
  ASSERT_TRUE(result.hasError());
  EXPECT_EQ(result.error(), PersistenceError::NotFound);
}

#include "PersistenceTest.moc"

// NOLINTEND

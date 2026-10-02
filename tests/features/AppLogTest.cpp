// NOLINTBEGIN
#include "MemoryPersistenceProvider.h"
#include "events/LogEvent.h"
#include "events/system/EventSystem.hpp"
#include "features/applog/applogcommon.h"
#include "features/applog/model/AppLogModel.h"
#include "features/applog/presenter/AppLogPresenter.h"
#include "features/applog/widget/AppLogWidget.h"

#include <QJsonArray>
#include <QJsonObject>
#include <QListWidget>
#include <QPushButton>
#include <QSignalSpy>
#include <QTest>
#include <gtest/gtest.h>

class AppLogTest : public ::testing::Test {
protected:
  MemoryPersistenceProvider provider;
  AppLogModel model;
  AppLogWidget view;
  AppLogPresenter presenter;

  AppLogTest()
      : model(provider, nullptr), view(nullptr), presenter(&model, &view) {}
};

TEST_F(AppLogTest, ModelEmitsLogChanged) {
  QSignalSpy spy(&model, &AppLogModel::logChanged);
  model.addLogMessage("Test message");

  ASSERT_EQ(spy.count(), 1);
  const auto args = spy.takeFirst();
  const LogDelta delta = qvariant_cast<LogDelta>(args.at(0));
  EXPECT_TRUE(delta.message.contains("Test message"));
}

TEST_F(AppLogTest, PresenterForwardsEvent_ModelEmits_ViewUpdates) {
  QSignalSpy modelSpy(&model, &AppLogModel::logChanged);

  LogEvent event;
  event.message = "Presenter test message";
  events::publish(event);

  QTest::qWait(1);

  ASSERT_EQ(modelSpy.count(), 1);

  auto *list = view.findChild<QListWidget *>("logListWidget");
  ASSERT_NE(list, nullptr);
  ASSERT_EQ(list->count(), 1);
  EXPECT_TRUE(list->item(0)->text().contains("Presenter test message"));
}

TEST_F(AppLogTest, ClearButtonClearsModelAndView) {
  model.addLogMessage("First log");
  model.addLogMessage("Second log");

  auto *list = view.findChild<QListWidget *>("logListWidget");
  ASSERT_NE(list, nullptr);
  ASSERT_EQ(list->count(), 2);

  QSignalSpy clearedSpy(&model, &AppLogModel::logCleared);

  auto *clearButton = view.findChild<QPushButton *>("clearButton");
  ASSERT_NE(clearButton, nullptr);
  QTest::mouseClick(clearButton, Qt::LeftButton);

  ASSERT_EQ(clearedSpy.count(), 1);
  EXPECT_EQ(list->count(), 0);
}

TEST_F(AppLogTest, TrimmingRemovesOldestBeyondMaxSize) {
  // Mirrors AppLogModel MAX_LOG_SIZE (src/features/applog/model/AppLogModel.cpp).
  constexpr int kMaxLogSize = 100;
  QSignalSpy spy(&model, &AppLogModel::logChanged);

  for (int i = 0; i < kMaxLogSize + 5; ++i) {
    model.addLogMessage(QString("msg_%1").arg(i));
  }

  ASSERT_EQ(model.getLogMessages().size(), kMaxLogSize);
  // Oldest entries removed; newest retained.
  EXPECT_TRUE(model.getLogMessages().first().contains("msg_5"));
  EXPECT_TRUE(model.getLogMessages().last().contains("msg_104"));

  ASSERT_EQ(spy.count(), kMaxLogSize + 5);

  // First emit is below the cap: not trimmed.
  const LogDelta firstDelta =
      qvariant_cast<LogDelta>(spy.at(0).at(0));
  EXPECT_FALSE(firstDelta.trimmed);
  EXPECT_TRUE(firstDelta.message.contains("msg_0"));

  // Overflow emits report trimming; the latest retains the newest message.
  const LogDelta midDelta =
      qvariant_cast<LogDelta>(spy.at(kMaxLogSize).at(0));
  EXPECT_TRUE(midDelta.trimmed);
  EXPECT_TRUE(midDelta.message.contains("msg_100"));

  const LogDelta lastDelta =
      qvariant_cast<LogDelta>(spy.at(spy.count() - 1).at(0));
  EXPECT_TRUE(lastDelta.trimmed);
  EXPECT_TRUE(lastDelta.message.contains("msg_104"));
}

TEST_F(AppLogTest, TrimmingPersistsOnlyRetainedState) {
  constexpr int kMaxLogSize = 100;
  MemoryPersistenceProvider localProvider;
  AppLogModel localModel(localProvider, nullptr);

  for (int i = 0; i < kMaxLogSize + 5; ++i) {
    localModel.addLogMessage(QString("persist_%1").arg(i));
  }
  ASSERT_EQ(localModel.getLogMessages().size(), kMaxLogSize);
  const auto saveResult = localModel.saveState();
  ASSERT_TRUE(saveResult.hasValue());

  AppLogModel reloaded(localProvider, nullptr);
  ASSERT_EQ(reloaded.getLogMessages().size(), kMaxLogSize);
  EXPECT_TRUE(reloaded.getLogMessages().first().contains("persist_5"));
  EXPECT_TRUE(reloaded.getLogMessages().last().contains("persist_104"));
}

// loadState must re-apply MAX_LOG_SIZE even when persisted state
// exceeds the cap (e.g. written by a prior version or external writer).
TEST_F(AppLogTest, LoadDoesNotExceedMaxLogSize) {
  constexpr int kMaxLogSize = 100;
  constexpr int kOverCap = 10;

  MemoryPersistenceProvider localProvider;

  // Write oversized state directly through the provider, bypassing runtime
  // trimming, to prove loadState re-applies the cap.
  QJsonArray oversized;
  for (int i = 0; i < kMaxLogSize + kOverCap; ++i) {
    oversized.append(QString("exceed_%1").arg(i));
  }
  QJsonObject obj;
  obj.insert("logMessages", oversized);
  const auto forced = localProvider.saveState(APP_ID ".AppLogState", obj);
  ASSERT_TRUE(forced.hasValue());

  AppLogModel reloaded(localProvider, nullptr);
  ASSERT_EQ(reloaded.getLogMessages().size(), kMaxLogSize);
  EXPECT_TRUE(reloaded.getLogMessages().first().contains("exceed_10"));
  EXPECT_TRUE(reloaded.getLogMessages().last().contains("exceed_109"));
}

// Failure injection through the model layer.
// Mirrors CounterTest OperationalLoadErrorLeavesDefaultState / SaveFailure
// IsForwardedToCaller against AppLogModel's PersistenceResult forwarding.
TEST_F(AppLogTest, OperationalLoadErrorLeavesDefaultState) {
  model.addLogMessage("Persisted before failure");
  ASSERT_TRUE(model.saveState().hasValue());

  provider.failNextLoad(PersistenceError::IoError);
  AppLogModel reloaded(provider, nullptr);
  EXPECT_TRUE(reloaded.getLogMessages().isEmpty());
}

TEST_F(AppLogTest, SaveFailureIsForwardedToCaller) {
  provider.failNextSave(PersistenceError::CommitError);
  const auto result = model.saveState();
  ASSERT_TRUE(result.hasError());
  EXPECT_EQ(result.error(), PersistenceError::CommitError);
}

#include "AppLogTest.moc"

// NOLINTEND

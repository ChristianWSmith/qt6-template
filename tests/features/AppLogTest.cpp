// NOLINTBEGIN
#include "MemoryPersistenceProvider.h"
#include "events/DemoLogEvent.h"
#include "events/system/EventSystem.hpp"
#include "features/applog/applogcommon.h"
#include "features/applog/model/AppLogModel.h"
#include "features/applog/presenter/AppLogPresenter.h"
#include "features/applog/widget/AppLogWidget.h"

#include <QJsonArray>
#include <QJsonObject>
#include <QListWidget>
#include <QPointer>
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

  DemoLogEvent event;
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

// Presenter initial model→view sync (AppLog analog of CounterTest): models
// load in ctor before the presenter exists; emissions during model
// construction have no subscribers. The presenter ctor must push current
// model state to the view once. This test locks that convention for restored
// (non-default) state without any user interaction.
TEST_F(AppLogTest, PresenterRestoresPersistedStateIntoViewWithoutInteraction) {
  MemoryPersistenceProvider seedProvider;
  {
    AppLogModel seed(seedProvider, nullptr);
    seed.addLogMessage("restored-one");
    seed.addLogMessage("restored-two");
    ASSERT_TRUE(seed.saveState().hasValue());
  }

  AppLogModel restored(seedProvider, nullptr);
  ASSERT_EQ(restored.getLogMessages().size(), 2);

  AppLogWidget view(nullptr);
  AppLogPresenter presenter(&restored, &view);

  auto *list = view.findChild<QListWidget *>("logListWidget");
  ASSERT_NE(list, nullptr);
  ASSERT_EQ(list->count(), 2);
  EXPECT_TRUE(list->item(0)->text().contains("restored-one"));
  EXPECT_TRUE(list->item(1)->text().contains("restored-two"));
}

// loadState must re-apply kMaxLogSize even when persisted state
// exceeds the cap (e.g. written by a prior version or external writer).
TEST_F(AppLogTest, LoadDoesNotExceedMaxLogSize) {
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

// Presenter QPointer hardening regression (AppLog analog of CounterTest).
//
// Destroy the model mid-session while widget + presenter remain live, then
// click clear. Presenter slots must null-guard via QPointer: no crash, no
// model path, view keeps last known state.
TEST_F(AppLogTest, PresenterGuardsNullModelAfterMidSessionDestruction) {
  auto *liveModel = new AppLogModel(provider, nullptr);
  auto *liveView = new AppLogWidget(nullptr);
  auto *livePresenter = new AppLogPresenter(liveModel, liveView, nullptr);

  liveModel->addLogMessage("before destroy");
  auto *list = liveView->findChild<QListWidget *>("logListWidget");
  ASSERT_NE(list, nullptr);
  ASSERT_EQ(list->count(), 1);

  auto *clearButton = liveView->findChild<QPushButton *>("clearButton");
  ASSERT_NE(clearButton, nullptr);

  QPointer<AppLogModel> observed(liveModel);
  delete liveModel;
  ASSERT_TRUE(observed.isNull());

  // Presenter slot must null-guard; model path must not run; view unchanged.
  QTest::mouseClick(clearButton, Qt::LeftButton);
  EXPECT_EQ(list->count(), 1);

  delete livePresenter;
  delete liveView;
  SUCCEED();
}

#include "AppLogTest.moc"

// NOLINTEND

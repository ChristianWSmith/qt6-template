// NOLINTBEGIN
#include "events/LogEvent.h"
#include "features/applog/applogcommon.h"
#include "features/applog/model/AppLogModel.h"
#include "features/applog/presenter/AppLogPresenter.h"
#include "features/applog/widget/AppLogWidget.h"

#include <QListWidget>
#include <QPushButton>
#include <QSignalSpy>
#include <QTest>
#include <gtest/gtest.h>

class TestLogEmitter : public QObject {
  Q_OBJECT
signals:
  void logEventReady(const LogEvent &);
};

class AppLogTest : public ::testing::Test {
protected:
  AppLogModel model;
  AppLogWidget view;
  AppLogPresenter presenter;

  AppLogTest() : model(nullptr), view(nullptr), presenter(&model, &view) {}
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
  TestLogEmitter emitter;
  QObject::connect(&emitter, &TestLogEmitter::logEventReady, &presenter,
                   [&](const LogEvent &event) {
                     QMetaObject::invokeMethod(&presenter, "onLogEventReceived",
                                               Qt::QueuedConnection,
                                               Q_ARG(LogEvent, event));
                   });

  QSignalSpy modelSpy(&model, &AppLogModel::logChanged);

  LogEvent event;
  event.message = "Presenter test message";
  emit emitter.logEventReady(event);

  QTest::qWait(100);

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

#include "AppLogTest.moc"

// NOLINTEND

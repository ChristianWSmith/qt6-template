// NOLINTBEGIN
#include "features/applog/model/AppLogModel.h"
#include "features/applog/applogcommon.h"
#include <QCoreApplication>
#include <QDateTime>
#include <QDebug>
#include <QSignalSpy>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

class AppLogModelTest : public ::testing::Test {
protected:
  AppLogModel *model = nullptr;

  void SetUp() override { model = new AppLogModel(); }

  void TearDown() override {
    delete model;
    model = nullptr;
  }
};

TEST_F(AppLogModelTest, ConstructorInstantiatesCorrectly) {
  ASSERT_NE(model, nullptr);
}

TEST_F(AppLogModelTest, AddLogMessageEmitsTimestampedSignal) {
  const QString raw = "This is a test log message";
  const LogDelta logDelta = LogDelta{raw, false};
  QSignalSpy spy(model, SIGNAL(logChanged(LogDelta)));
  ASSERT_TRUE(spy.isValid());

  model->addLogMessage(raw);

  ASSERT_EQ(spy.count(), 1);
  const LogDelta emitted = qvariant_cast<LogDelta>(spy.takeFirst().at(0));

  QDateTime now = QDateTime::currentDateTime();
  QString date = now.toString("yyyy-MM-dd");
  QString time = now.toString("hh:mm:ss");

  EXPECT_TRUE(emitted.message.startsWith(date));
  EXPECT_TRUE(emitted.message.contains(time));
  EXPECT_TRUE(emitted.message.contains(" - "));
  EXPECT_TRUE(emitted.message.endsWith(raw));
}

TEST_F(AppLogModelTest, ConnectLogChangedReturnsValidConnection) {
  QObject dummy;
  QMetaObject::Connection conn =
      model->connectLogChanged(&dummy, SLOT(deleteLater()));
  EXPECT_NE(conn, QMetaObject::Connection());
  QObject::disconnect(conn);
}
// NOLINTEND

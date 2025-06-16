#include "features/applog/model/AppLogModel.h"
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
  QSignalSpy spy(model, SIGNAL(logMessageAdded(QString)));
  ASSERT_TRUE(spy.isValid());

  model->addLogMessage(raw);

  ASSERT_EQ(spy.count(), 1);
  const QString emitted = spy.takeFirst().at(0).toString();

  QDateTime now = QDateTime::currentDateTime();
  QString date = now.toString("yyyy-MM-dd");
  QString time = now.toString("hh:mm:ss");

  EXPECT_TRUE(emitted.startsWith(date));
  EXPECT_TRUE(emitted.contains(time));
  EXPECT_TRUE(emitted.contains(" - "));
  EXPECT_TRUE(emitted.endsWith(raw));
}

TEST_F(AppLogModelTest, ConnectLogMessageAddedReturnsValidConnection) {
  QObject dummy;
  QMetaObject::Connection conn =
      model->connectLogMessageAdded(&dummy, SLOT(deleteLater()));
  EXPECT_NE(conn, QMetaObject::Connection());
  QObject::disconnect(conn);
}

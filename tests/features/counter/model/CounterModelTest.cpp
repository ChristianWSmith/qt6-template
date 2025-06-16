#include "features/counter/model/CounterModel.h"
#include "gtest/gtest.h"
#include <QCoreApplication>
#include <QSignalSpy>

class CounterModelTest : public ::testing::Test {
protected:
  CounterModel *model;

  void SetUp() override { model = new CounterModel(); }

  void TearDown() override { delete model; }
};

TEST_F(CounterModelTest, ConstructorInitializesValueToZero) {
  EXPECT_EQ(model->value(), 0);
}

TEST_F(CounterModelTest, IncrementIncreasesValueAndEmitsSignal) {
  QSignalSpy spy(model, SIGNAL(valueChanged(int)));
  ASSERT_TRUE(spy.isValid())
      << "Failed to connect QSignalSpy to valueChanged(int).";

  model->increment();

  ASSERT_EQ(spy.count(), 1);
  QList<QVariant> args = spy.takeFirst();
  ASSERT_EQ(args.size(), 1);
  EXPECT_EQ(args.at(0).toInt(), 1);
  EXPECT_EQ(model->value(), 1);
}

TEST_F(CounterModelTest, ConnectValueChangedReturnsValidConnection) {
  QObject dummy;
  QMetaObject::Connection connection =
      model->connectValueChanged(&dummy, SLOT(deleteLater()));

  EXPECT_NE(connection, QMetaObject::Connection());
  QObject::disconnect(connection);
}

TEST_F(CounterModelTest, IncrementMultipleTimesUpdatesValueCorrectly) {
  QSignalSpy spy(model, SIGNAL(valueChanged(int)));
  model->increment();
  model->increment();
  model->increment();

  EXPECT_EQ(model->value(), 3);
  EXPECT_EQ(spy.count(), 3);

  QList<QVariant> lastArgs = spy.takeLast();
  EXPECT_EQ(lastArgs.at(0).toInt(), 3);
}

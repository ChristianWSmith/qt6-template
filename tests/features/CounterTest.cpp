// NOLINTBEGIN
#include "features/counter/model/CounterModel.h"
#include "features/counter/presenter/CounterPresenter.h"
#include "features/counter/widget/CounterWidget.h"

#include <QLabel>
#include <QPushButton>
#include <QSignalSpy>
#include <QTest>
#include <gtest/gtest.h>

class CounterTest : public ::testing::Test {
protected:
  CounterModel model;
  CounterWidget view;
  CounterPresenter presenter;

  CounterTest()
      : model(nullptr, nullptr), view(nullptr), presenter(&model, &view) {}
};

TEST_F(CounterTest, ModelStartsAtZero) { EXPECT_EQ(model.value(), 0); }

TEST_F(CounterTest, IncrementEmitsValueChangedAndUpdatesView) {
  QSignalSpy spy(&model, &CounterModel::valueChanged);

  auto *label = view.findChild<QLabel *>("counterLabel");
  ASSERT_NE(label, nullptr);
  EXPECT_EQ(label->text(), "0");

  model.increment();

  ASSERT_EQ(spy.count(), 1);
  EXPECT_EQ(spy.takeFirst().at(0).toInt(), 1);

  EXPECT_EQ(label->text(), "1");
}

TEST_F(CounterTest, ResetEmitsValueChangedAndUpdatesView) {
  model.increment();
  ASSERT_EQ(model.value(), 1);

  QSignalSpy spy(&model, &CounterModel::valueChanged);

  model.reset();

  ASSERT_EQ(spy.count(), 1);
  EXPECT_EQ(spy.takeFirst().at(0).toInt(), 0);

  auto *label = view.findChild<QLabel *>("counterLabel");
  ASSERT_NE(label, nullptr);
  EXPECT_EQ(label->text(), "0");
}

TEST_F(CounterTest, ClickingIncrementButtonIncrementsModelAndView) {
  QSignalSpy spy(&model, &CounterModel::valueChanged);

  auto *button = view.findChild<QPushButton *>("incrementButton");
  ASSERT_NE(button, nullptr);

  QTest::mouseClick(button, Qt::LeftButton);

  ASSERT_EQ(spy.count(), 1);
  EXPECT_EQ(model.value(), 1);

  auto *label = view.findChild<QLabel *>("counterLabel");
  ASSERT_NE(label, nullptr);
  EXPECT_EQ(label->text(), "1");
}

TEST_F(CounterTest, ClickingResetButtonResetsModelAndView) {
  model.increment();
  ASSERT_EQ(model.value(), 1);

  QSignalSpy spy(&model, &CounterModel::valueChanged);

  auto *button = view.findChild<QPushButton *>("resetButton");
  ASSERT_NE(button, nullptr);

  QTest::mouseClick(button, Qt::LeftButton);

  ASSERT_EQ(spy.count(), 1);
  EXPECT_EQ(model.value(), 0);

  auto *label = view.findChild<QLabel *>("counterLabel");
  ASSERT_NE(label, nullptr);
  EXPECT_EQ(label->text(), "0");
}

#include "CounterTest.moc"

// NOLINTEND

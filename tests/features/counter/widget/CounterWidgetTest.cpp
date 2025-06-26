// NOLINTBEGIN
#include "features/counter/widget/CounterWidget.h"
#include <QCoreApplication>
#include <QDebug>
#include <QSignalSpy>
#include <QTimer>
#include <gtest/gtest.h>

class CounterWidgetTest : public ::testing::Test {
protected:
  CounterWidget *widget = nullptr;

  void SetUp() override {
    widget = new CounterWidget();
    widget->show();
  }

  void TearDown() override {
    widget->hide();
    delete widget;
  }
};

TEST_F(CounterWidgetTest, DisplayCounterUpdatesLabel) {
  auto *label = widget->findChild<QLabel *>("counterLabel");
  ASSERT_NE(label, nullptr);

  widget->displayCounter(42);

  EXPECT_EQ(label->text(), "42");
}

TEST_F(CounterWidgetTest, ClickingIncrementEmitsSignal) {
  QSignalSpy spy(widget, SIGNAL(incrementRequested()));

  QMetaObject::invokeMethod(widget, "on_incrementButton_clicked",
                            Qt::DirectConnection);

  ASSERT_EQ(spy.count(), 1);
}

// NOLINTEND

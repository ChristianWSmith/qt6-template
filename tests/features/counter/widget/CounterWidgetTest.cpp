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

TEST_F(CounterWidgetTest, ConnectIncrementRequestedBindsSignal) {
  qDebug() << "Starting ConnectIncrementRequestedBindsSignal test";

  QObject *receiver = new QObject();

  bool wasCalled = false;
  QObject::connect(receiver, &QObject::destroyed, [&wasCalled]() {
    qDebug() << "Receiver destroyed signal caught";
    wasCalled = true;
  });

  auto conn = QObject::connect(widget, &CounterWidget::incrementRequested,
                               receiver, &QObject::deleteLater);

  qDebug() << "Connection success:" << (conn != QMetaObject::Connection());

  EXPECT_TRUE(conn != QMetaObject::Connection());

  qDebug() << "Emitting incrementRequested signal";
  emit widget->incrementRequested();

  qDebug() << "Processing events";
  QEventLoop loop;
  QTimer::singleShot(50, &loop, &QEventLoop::quit);
  loop.exec();

  qDebug() << "Was called:" << wasCalled;

  EXPECT_TRUE(wasCalled);

  if (!wasCalled) {
    delete receiver;
  }
}
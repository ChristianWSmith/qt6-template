#include "features/applog/widget/AppLogWidget.h"
#include <QScrollBar>
#include <QTextEdit>
#include <gtest/gtest.h>

class AppLogWidgetTest : public ::testing::Test {
protected:
  AppLogWidget *widget = nullptr;

  void SetUp() override {
    widget = new AppLogWidget();
    widget->show(); // Ensure widget layouting and scrollbar behavior
  }

  void TearDown() override {
    widget->hide();
    delete widget;
  }
};

TEST_F(AppLogWidgetTest, InstantiatesSuccessfully) {
  ASSERT_NE(widget, nullptr);
  ASSERT_NE(widget->findChild<QTextEdit *>(), nullptr);
}

TEST_F(AppLogWidgetTest, AppendsLogMessage) {
  auto *textEdit = widget->findChild<QTextEdit *>();
  ASSERT_NE(textEdit, nullptr);

  int beforeCount = textEdit->document()->blockCount();

  widget->displayLogMessage("123");
  widget->displayLogMessage("456");

  int afterCount = textEdit->document()->blockCount();
  EXPECT_EQ(afterCount, beforeCount + 1);
  EXPECT_TRUE(textEdit->toPlainText().contains("123\n456"));
}

TEST_F(AppLogWidgetTest, ScrollsToBottomOnLogMessage) {
  auto *textEdit = widget->findChild<QTextEdit *>();
  ASSERT_NE(textEdit, nullptr);

  QScrollBar *scrollBar = textEdit->verticalScrollBar();
  ASSERT_NE(scrollBar, nullptr);

  for (int i = 0; i < 100; ++i) {
    widget->displayLogMessage(QString("Line %1").arg(i));
  }

  scrollBar->setValue(0); // Manually scroll to top

  widget->displayLogMessage("Final log line");

  EXPECT_EQ(scrollBar->value(), scrollBar->maximum());
}

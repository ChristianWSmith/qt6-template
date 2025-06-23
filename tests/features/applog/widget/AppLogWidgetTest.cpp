// NOLINTBEGIN
#include "features/applog/widget/AppLogWidget.h"
#include <QListWidget>
#include <QScrollBar>
#include <gtest/gtest.h>

class AppLogWidgetTest : public ::testing::Test {
protected:
  AppLogWidget *widget = nullptr;

  void SetUp() override {
    widget = new AppLogWidget();
    widget->show();
  }

  void TearDown() override {
    widget->hide();
    delete widget;
  }
};

TEST_F(AppLogWidgetTest, InstantiatesSuccessfully) {
  ASSERT_NE(widget, nullptr);
  ASSERT_NE(widget->findChild<QListWidget *>(), nullptr);
}

TEST_F(AppLogWidgetTest, AppendsLogMessage) {
  auto *listWidget = widget->findChild<QListWidget *>();
  ASSERT_NE(listWidget, nullptr);

  int beforeCount = listWidget->count();

  widget->handleLogChanged(LogDelta{"123", false});
  widget->handleLogChanged(LogDelta{"456", false});

  int afterCount = listWidget->count();
  EXPECT_EQ(afterCount, beforeCount + 2);

  // Verify the contents of the last two added items
  ASSERT_GE(afterCount, 2);
  EXPECT_EQ(listWidget->item(afterCount - 2)->text(), "123");
  EXPECT_EQ(listWidget->item(afterCount - 1)->text(), "456");
}

TEST_F(AppLogWidgetTest, ScrollsToBottomOnLogMessage) {
  auto *listWidget = widget->findChild<QListWidget *>();
  ASSERT_NE(listWidget, nullptr);

  QScrollBar *scrollBar = listWidget->verticalScrollBar();
  ASSERT_NE(scrollBar, nullptr);

  // Fill enough items to require scrolling
  for (int i = 0; i < 100; ++i) {
    widget->handleLogChanged(LogDelta{QString("Line %1").arg(i), false});
  }

  // Scroll to top manually
  scrollBar->setValue(0);
  widget->handleLogChanged(LogDelta{"Final log line", false});
  EXPECT_EQ(scrollBar->value(), 0);

  scrollBar->setValue(scrollBar->maximum());
  // Add one more log message
  widget->handleLogChanged(LogDelta{"Final log line", false});

  // Expect scroll bar to be at the bottom
  EXPECT_EQ(scrollBar->value(), scrollBar->maximum());
}
// NOLINTEND

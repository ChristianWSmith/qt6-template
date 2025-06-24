// NOLINTBEGIN
#include "widgets/reusable/ReusableWidget.h"
#include <gtest/gtest.h>

class ReusableWidgetTest : public ::testing::Test {
protected:
  ReusableWidget *widget = nullptr;

  void SetUp() override {
    widget = new ReusableWidget();
    widget->show();
  }

  void TearDown() override {
    widget->hide();
    delete widget;
  }
};

// Add your widget tests here
TEST_F(ReusableWidgetTest, Placeholder) { EXPECT_TRUE(true); }
// NOLINTEND

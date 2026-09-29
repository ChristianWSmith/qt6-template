// NOLINTBEGIN
#include "appmainwindow/AppMainWindow.h"

#include <QApplication>
#include <QCloseEvent>
#include <QSettings>
#include <gtest/gtest.h>

class LifecycleTest : public ::testing::Test {};

TEST_F(LifecycleTest, NormalCloseDoesNotCrash) {
  AppMainWindow window;
  window.show();
  QApplication::processEvents();

  QCloseEvent closeEvent;
  QApplication::sendEvent(&window, &closeEvent);

  EXPECT_TRUE(closeEvent.isAccepted());
}

TEST_F(LifecycleTest, CloseSavesGeometry) {
  {
    AppMainWindow window;
    window.resize(640, 480);
    window.show();
    QApplication::processEvents();

    QCloseEvent closeEvent;
    QApplication::sendEvent(&window, &closeEvent);
    EXPECT_TRUE(closeEvent.isAccepted());
  }

  QSettings settings(ORGANIZATION_NAME, APP_NAME);
  EXPECT_FALSE(settings.value("window/geometry").toByteArray().isEmpty());
}

TEST_F(LifecycleTest, CloseSavesModelState) {
  {
    AppMainWindow window;
    window.show();
    QApplication::processEvents();

    QCloseEvent closeEvent;
    QApplication::sendEvent(&window, &closeEvent);
    EXPECT_TRUE(closeEvent.isAccepted());
  }

  QSettings settings(ORGANIZATION_NAME, APP_NAME);
  EXPECT_FALSE(settings.value("window/state").toByteArray().isEmpty());
}

#include "LifecycleTest.moc"

// NOLINTEND

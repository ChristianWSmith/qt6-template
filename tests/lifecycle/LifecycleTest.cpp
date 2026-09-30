// NOLINTBEGIN
#include "appmainwindow/AppMainWindow.h"
#include "platform/core/FilePersistenceProvider.h"

#include <QApplication>
#include <QCloseEvent>
#include <QFile>
#include <QPushButton>
#include <QSettings>
#include <QStandardPaths>
#include <QTest>
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

TEST_F(LifecycleTest, CloseSavesWindowState) {
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

// F-12: closeEvent must persist feature state end-to-end via the polymorphic
// IModel list (F-09) and PersistenceResult propagation (F-01).
TEST_F(LifecycleTest, CloseSavesFeatureStateEndToEnd) {
  const QString counterKey = QStringLiteral(APP_ID ".CounterState");
  const QString counterPath =
      QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) +
      QLatin1Char('/') + counterKey + QStringLiteral(".json");
  // Start from first-run state so the assertion does not depend on leftover
  // QStandardPaths test-mode files from prior test runs.
  QFile::remove(counterPath);

  {
    AppMainWindow window;
    window.show();
    QApplication::processEvents();

    // Mutate counter state through the UI (widget → presenter → model).
    auto *button =
        window.findChild<QPushButton *>("incrementButton");
    ASSERT_NE(button, nullptr);
    QTest::mouseClick(button, Qt::LeftButton);
    QApplication::processEvents();

    QCloseEvent closeEvent;
    QApplication::sendEvent(&window, &closeEvent);
    EXPECT_TRUE(closeEvent.isAccepted());
  }

  // Verify the persisted file independently via a fresh provider.
  FilePersistenceProvider verifier;
  auto loadResult = verifier.loadState(counterKey);
  ASSERT_TRUE(loadResult.hasValue())
      << "counter state missing after closeEvent";
  EXPECT_EQ(loadResult.value().value("value").toInt(), 1);
}

#include "LifecycleTest.moc"

// NOLINTEND

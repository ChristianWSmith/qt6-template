// NOLINTBEGIN
#include "appmainwindow/AppMainWindow.h"
#include "platform/core/FilePersistenceProvider.h"

#include "features/applog/model/AppLogModel.h"
#include "features/applog/presenter/AppLogPresenter.h"
#include "features/applog/widget/AppLogWidget.h"
#include "features/counter/model/CounterModel.h"
#include "features/counter/presenter/CounterPresenter.h"
#include "features/counter/widget/CounterWidget.h"

#include <QApplication>
#include <QCloseEvent>
#include <QFile>
#include <QPushButton>
#include <QSettings>
#include <QStandardPaths>
#include <QTest>
#include <gtest/gtest.h>

class LifecycleTest : public ::testing::Test {};

// F-03 (Wave 3, simple test — not a framework): composition-root wiring
// must actually construct the documented feature graph. Missing wiring
// (model/widget/presenter/provider) is a silent-failure class this locks.
TEST_F(LifecycleTest, CompositionRootConstructsFeatureGraph) {
  AppMainWindow window;

  auto *central = window.centralWidget();
  ASSERT_NE(central, nullptr);

  EXPECT_EQ(window.findChildren<FilePersistenceProvider *>().size(), 1);
  EXPECT_EQ(window.findChildren<CounterModel *>().size(), 1);
  EXPECT_EQ(window.findChildren<CounterWidget *>().size(), 1);
  EXPECT_EQ(window.findChildren<CounterPresenter *>().size(), 1);
  EXPECT_EQ(window.findChildren<AppLogModel *>().size(), 1);
  EXPECT_EQ(window.findChildren<AppLogWidget *>().size(), 1);
  EXPECT_EQ(window.findChildren<AppLogPresenter *>().size(), 1);

  // Feature widgets must be in the window's object tree (layout reparents
  // them under the central container, which is parented to the window).
  EXPECT_TRUE(window.isAncestorOf(window.findChild<CounterWidget *>()));
  EXPECT_TRUE(window.isAncestorOf(window.findChild<AppLogWidget *>()));
}

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

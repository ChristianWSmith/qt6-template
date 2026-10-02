// NOLINTBEGIN
#include "appmainwindow/AppMainWindow.h"
#include "platform/core/FilePersistenceProvider.h"

#include "features/applog/model/AppLogModel.h"
#include "features/applog/presenter/AppLogPresenter.h"
#include "features/applog/widget/AppLogWidget.h"
#include "features/counter/model/CounterModel.h"
#include "features/counter/presenter/CounterPresenter.h"
#include "features/counter/widget/CounterWidget.h"

#include "MemoryPersistenceProvider.h"

#include <QApplication>
#include <QCloseEvent>
#include <QFile>
#include <QPushButton>
#include <QSettings>
#include <QStandardPaths>
#include <QTest>
#include <gtest/gtest.h>

class LifecycleTest : public ::testing::Test {};

// Simple test — not a framework: composition-root wiring
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

// OWN-012: AppMainWindow is stack-allocated in main(); setting
// Qt::WA_DeleteOnClose would double-destroy the window.
TEST_F(LifecycleTest, StackWindowDoesNotSetDeleteOnClose) {
  AppMainWindow window;
  EXPECT_FALSE(window.testAttribute(Qt::WA_DeleteOnClose));
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

// closeEvent must persist feature state end-to-end via the polymorphic
// IModel list and PersistenceResult propagation.
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
    auto *button = window.findChild<QPushButton *>("incrementButton");
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

// external persistence provider injection seam. When the injected ctor is
// used it must bind the caller-owned provider (non-owning; no internal
// FilePersistenceProvider is constructed). A failing save at closeEvent must
// be observed by the composition root and must NOT abort close — the
// log-and-continue policy is unchanged.
TEST_F(LifecycleTest, CloseContinuesWhenInjectedProviderSaveFails) {
  MemoryPersistenceProvider provider;
  provider.failNextSave(PersistenceError::CommitError);

  // Provider declared before the window so it outlives it (destruction order
  // is reverse of declaration; the injected provider is caller-owned).
  // Injected overload: borrow caller-owned provider (reference binding).
  AppMainWindow window(provider);
  window.show();
  QApplication::processEvents();

  // Injected provider path: no internal FilePersistenceProvider child.
  EXPECT_EQ(window.findChildren<FilePersistenceProvider *>().size(), 0);

  QCloseEvent closeEvent;
  QApplication::sendEvent(&window, &closeEvent);

  // Contract: save failure → result observed → diagnostic → close continues.
  // Close must be accepted (not rejected/aborted) and the window hidden.
  EXPECT_TRUE(closeEvent.isAccepted());
  EXPECT_TRUE(window.isHidden());
}

// Production path (AUD-001 Option B): closeEvent must still drive the m_models
// loop and deliver feature state to the caller-owned injected provider.
TEST_F(LifecycleTest, InjectedProviderReceivesFeatureStateOnClose) {
  MemoryPersistenceProvider provider;

  {
    AppMainWindow window(provider);
    window.show();
    QApplication::processEvents();

    auto *button = window.findChild<QPushButton *>("incrementButton");
    ASSERT_NE(button, nullptr);
    QTest::mouseClick(button, Qt::LeftButton);
    QApplication::processEvents();

    QCloseEvent closeEvent;
    QApplication::sendEvent(&window, &closeEvent);
    EXPECT_TRUE(closeEvent.isAccepted());
  }

  auto loadResult = provider.loadState(QStringLiteral(APP_ID ".CounterState"));
  ASSERT_TRUE(loadResult.hasValue())
      << "counter state not saved to injected provider via m_models loop";
  EXPECT_EQ(loadResult.value().value("value").toInt(), 1);
}

// QSettings restore path: ctor restoreGeometry/restoreState must apply
// chrome saved by a prior close (save-side is covered by CloseSavesGeometry).
TEST_F(LifecycleTest, CtorRestoresGeometryFromQSettings) {
  const QSize targetSize(640, 480);
  {
    AppMainWindow window;
    window.resize(targetSize);
    window.show();
    QApplication::processEvents();

    QSettings settings(ORGANIZATION_NAME, APP_NAME);
    settings.setValue("window/geometry", window.saveGeometry());
    settings.setValue("window/state", window.saveState());
  }

  AppMainWindow restored;
  restored.show();
  QApplication::processEvents();

  EXPECT_EQ(restored.size(), targetSize);
}

#include "LifecycleTest.moc"

// NOLINTEND

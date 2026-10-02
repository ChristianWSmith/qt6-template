// NOLINTBEGIN
#include "MemoryPersistenceProvider.h"
#include "features/counter/model/CounterModel.h"
#include "features/counter/presenter/CounterPresenter.h"
#include "features/counter/widget/CounterWidget.h"

#include <QLabel>
#include <QPointer>
#include <QPushButton>
#include <QSignalSpy>
#include <QTest>
#include <gtest/gtest.h>

class CounterTest : public ::testing::Test {
protected:
  MemoryPersistenceProvider provider;
  CounterModel model;
  CounterWidget view;
  CounterPresenter presenter;

  CounterTest()
      : model(provider, nullptr), view(nullptr), presenter(model, view) {}
};

TEST_F(CounterTest, ModelStartsAtZero) { EXPECT_EQ(model.value(), 0); }

TEST_F(CounterTest, SaveStateReturnsSuccessAndPersistsValue) {
  model.increment();
  const auto result = model.saveState();
  ASSERT_TRUE(result.hasValue());

  CounterModel reloaded(provider, nullptr);
  EXPECT_EQ(reloaded.value(), 1);
}

// Presenter initial model→view sync: models load in ctor before the presenter
// exists; emissions during model construction have no subscribers. The presenter
// ctor must push current model state to the view once. This test locks that
// convention for restored (non-default) state without any user interaction.
TEST_F(CounterTest, PresenterRestoresPersistedStateIntoViewWithoutInteraction) {
  MemoryPersistenceProvider seedProvider;
  {
    CounterModel seed(seedProvider, nullptr);
    for (int i = 0; i < 5; ++i) {
      seed.increment();
    }
    ASSERT_EQ(seed.value(), 5);
    ASSERT_TRUE(seed.saveState().hasValue());
  }

  CounterModel restored(seedProvider, nullptr);
  ASSERT_EQ(restored.value(), 5);

  CounterWidget view(nullptr);
  CounterPresenter presenter(restored, view);

  auto *label = view.findChild<QLabel *>("counterLabel");
  ASSERT_NE(label, nullptr);
  EXPECT_EQ(label->text(), "5");
}

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

// Presenter teardown discipline.
//
// This test establishes the template's presenter destructor contract:
// presenter destruction must not require its non-owning dependencies to
// remain alive. It deliberately destroys view and model BEFORE the presenter,
// mirroring production-like Qt reverse-deletion of represed widgets.
//
// This test does NOT claim to establish a Qt destruction-order guarantee.
// It only proves the presenter does not dereference m_model/m_view in its
// destructor (connections auto-disconnect via Qt).
TEST(CounterTeardownTest, PresenterSurvivesDependencyDestruction) {
  MemoryPersistenceProvider provider;
  auto *model = new CounterModel(provider, nullptr);
  auto *view = new CounterWidget(nullptr);
  auto *presenter = new CounterPresenter(*model, *view, nullptr);

  // Establish live signal/slot connections so destruction has something to
  // tear down.
  model->increment();
  ASSERT_EQ(model->value(), 1);

  delete view;
  delete model;
  // Presenter dtor must not crash despite dangling non-owning pointers.
  delete presenter;
  SUCCEED();
}

// Presenter QPointer hardening regression.
//
// Destroy the model mid-session while widget + presenter remain live, then
// click increment. Presenter slots must null-guard via QPointer: no crash,
// no model mutation, and the view keeps its last known state.
TEST_F(CounterTest, PresenterGuardsNullModelAfterMidSessionDestruction) {
  auto *liveModel = new CounterModel(provider, nullptr);
  auto *liveView = new CounterWidget(nullptr);
  auto *livePresenter = new CounterPresenter(*liveModel, *liveView, nullptr);

  liveModel->increment();
  ASSERT_EQ(liveModel->value(), 1);

  auto *liveLabel = liveView->findChild<QLabel *>("counterLabel");
  ASSERT_NE(liveLabel, nullptr);
  EXPECT_EQ(liveLabel->text(), "1");

  auto *liveButton = liveView->findChild<QPushButton *>("incrementButton");
  ASSERT_NE(liveButton, nullptr);

  // QPointer observability: the test-side handle reports null on destruction.
  QPointer<CounterModel> observed(liveModel);
  delete liveModel;
  ASSERT_TRUE(observed.isNull());

  // Presenter slot must null-guard; view must not be mutated further.
  QTest::mouseClick(liveButton, Qt::LeftButton);
  EXPECT_EQ(liveLabel->text(), "1");

  delete livePresenter;
  delete liveView;
  SUCCEED();
}

// Failure injection through the model layer.
TEST_F(CounterTest, OperationalLoadErrorLeavesDefaultState) {
  model.increment();
  ASSERT_TRUE(model.saveState().hasValue());

  provider.failNextLoad(PersistenceError::IoError);
  CounterModel reloaded(provider, nullptr);
  EXPECT_EQ(reloaded.value(), 0);
}

TEST_F(CounterTest, SaveFailureIsForwardedToCaller) {
  provider.failNextSave(PersistenceError::CommitError);
  const auto result = model.saveState();
  ASSERT_TRUE(result.hasError());
  EXPECT_EQ(result.error(), PersistenceError::CommitError);
}

#include "CounterTest.moc"

// NOLINTEND

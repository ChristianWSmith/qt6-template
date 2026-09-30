// NOLINTBEGIN
#include "MemoryPersistenceProvider.h"
#include "features/counter/model/CounterModel.h"
#include "features/counter/presenter/CounterPresenter.h"
#include "features/counter/widget/CounterWidget.h"

#include <QLabel>
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
      : model(provider, nullptr), view(nullptr), presenter(&model, &view) {}
};

TEST_F(CounterTest, ModelStartsAtZero) { EXPECT_EQ(model.value(), 0); }

TEST_F(CounterTest, SaveStateReturnsSuccessAndPersistsValue) {
  model.increment();
  const auto result = model.saveState();
  ASSERT_TRUE(result.hasValue());

  CounterModel reloaded(provider, nullptr);
  EXPECT_EQ(reloaded.value(), 1);
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

// F-15 — Presenter teardown discipline.
//
// This test establishes the template's presenter destructor contract:
// presenter destruction must not require its non-owning dependencies to
// remain alive. It deliberately destroys view and model BEFORE the presenter,
// mirroring production-like Qt reverse-deletion of reparented widgets.
//
// This test does NOT claim to establish a Qt destruction-order guarantee.
// It only proves the presenter does not dereference m_model/m_view in its
// destructor (connections auto-disconnect via Qt).
TEST(CounterTeardownTest, PresenterSurvivesDependencyDestruction) {
  MemoryPersistenceProvider provider;
  auto *model = new CounterModel(provider, nullptr);
  auto *view = new CounterWidget(nullptr);
  auto *presenter = new CounterPresenter(model, view, nullptr);

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

#include "CounterTest.moc"

// NOLINTEND
